#include "pch.h"

#include "parse.h"

#include "PreprocessSource.h"
#include "ReadFile.h"
#include "ReadFileAsLines.h"
#include "TokenSearch.h"
#include "WriteFile.h"
#include "data.h"
#include "fn_ExtractIdentifier.h"
#include "sc_Args.h"

namespace fs = std::filesystem;

// Resolve `pathsToDo` argument to list of files.
std::optional<std::vector<fs::path>> ResolveArgsSources(std::vector<fs::path> pathsToDo,
                                                        bool verbose) {
    std::set<fs::path> pathsDone;
    std::vector<fs::path> sources;

    while (!pathsToDo.empty()) {
        auto f = std::move(pathsToDo.back());
        pathsToDo.pop_back();
        std::error_code ec;
        auto cf = fs::canonical(f, ec);
        if (ec) {
            fmt::print(stderr, "File doesn't exist: `{}`\n", f);
            return std::nullopt;
        }
        if (pathsDone.contains(cf)) {
            // Duplicate.
            continue;
        }
        pathsDone.insert(cf);
        auto ext = cf.extension();
        if (k_validSourceExtensions.contains(ext)) {
            sources.push_back(cf);
        } else if (k_fileListExtensions.contains(ext)) {
            auto pathList = ReadFileAsLines(f);
            if (!pathList) {
                fmt::print(stderr, "Can't read file list from `{}`.\n", f);
                return std::nullopt;
            }
            pathsToDo.reserve(pathsToDo.size() + pathList->size());
            std::optional<fs::path> base;
            if (f.has_parent_path()) {
                base = f.parent_path();
            }
            for (auto& p : *pathList) {
                if (!p.empty()) {
                    if (base) {
                        p = *base / p;
                    }
                    pathsToDo.push_back(std::move(p));
                }
            }
        } else {
            // Ignore this file.
            if (verbose) {
                fmt::print(stderr, "Ignoring file with extension `{}`: {}", ext, f);
            }
        }
    }

    std::sort(sources.begin(), sources.end());
    return sources;
}

std::string JoinTokensWithoutComments(
    std::span<const libtokenizer::Token> tokens,
    std::span<const libtokenizer::Token*> exceptTokens = std::span<const libtokenizer::Token*>()) {
    std::string r;
    std::optional<std::string_view> continuousTokens;
    auto flushContinuousTokens = [&continuousTokens, &r]() {
        if (continuousTokens) {
            if (r.empty()) {
                r = std::string(*continuousTokens);
            } else {
                r += fmt::format(" {}", *continuousTokens);
            }
        }
        continuousTokens.reset();
    };
    for (auto& t : tokens) {
        if (t.IsComment() || std::ranges::contains(exceptTokens, &t)) {
            flushContinuousTokens();
        } else {
            if (continuousTokens) {
                continuousTokens =
                    std::string_view(continuousTokens->data(), data_plus_size(t.sourceValue));
            } else {
                continuousTokens = t.sourceValue;
            }
        }
    }
    flushContinuousTokens();
    return r;
}

// Expected: ... <name> `(` ... `)` ... `{` ... `}` for free functions, or
//           ... <classname> `::` <name> `(` ... `)` ... `{` ... `}` for member functions.
std::expected<std::string, std::string> ExtractFunctionDeclaration(
    std::optional<std::string_view> className,
    std::string_view name,
    std::span<const libtokenizer::Token> tokens0) {
    using libtokenizer::Token;
    using libtokenizer::TokenType;
    size_t openingParenIdx = SIZE_T_MAX;
    auto tokens = tokens0;
    std::optional<std::array<const Token*, 2>> classNameAndDoubleColonTokens;
    for (;;) {
        auto search = TokenSearch(tokens);
        size_t firstTokenIdx = SIZE_T_MAX;
        if (className) {
            size_t classNameIdx = SIZE_T_MAX;
            size_t doubleColonIdx = SIZE_T_MAX;
            search.Find(TokenType::id, *className, &classNameIdx)
                .Eat(TokenType::tok, "::", &doubleColonIdx)
                .Eat(TokenType::id, name);
            if (classNameIdx != SIZE_T_MAX && doubleColonIdx != SIZE_T_MAX) {
                classNameAndDoubleColonTokens.emplace();
                (*classNameAndDoubleColonTokens)[0] = &tokens[classNameIdx];
                (*classNameAndDoubleColonTokens)[1] = &tokens[doubleColonIdx];
            }
            firstTokenIdx = classNameIdx;
        } else {
            size_t nameIdx = SIZE_T_MAX;
            search.Find(TokenType::id, name, &nameIdx);
            firstTokenIdx = nameIdx;
        }
        auto searchResult =
            std::move(search.Eat(TokenType::tok, "(", &openingParenIdx)).FinishSearch();
        if (firstTokenIdx == SIZE_T_MAX) {
            // Even the `className` or `name` token wasn't found, no need to
            // backtrack.
            return std::unexpected(
                className ? fmt::format(
                    "Pattern <class-name>::<memfn-name> not found ({}::{})", className, name)
                          : fmt::format("Pattern <fn-name> not found ({})", name));
        }
        if (searchResult.has_value()) {
            // The `(` was also found.
            break;
        }
        // Backtract, start over from where the clasName was found.
        tokens = tokens.subspan(firstTokenIdx + 1);
    }
    if (openingParenIdx == SIZE_T_MAX) {
        return std::unexpected(fmt::format("Can't find `(`"));
    }
    tokens = tokens.subspan(openingParenIdx);
    size_t openingBraceIdx = SIZE_T_MAX;
    auto searchResult = std::move(TokenSearch(tokens)
                                      .Eat(TokenType::tok, "(")
                                      .Eat(TokenType::tok, ")")
                                      .Find(TokenType::tok, "{", &openingBraceIdx)
                                      .Eat(TokenType::tok, "}")
                                      .AssertEnd())
                            .FinishSearch();
    if (!searchResult.has_value()) {
        return std::unexpected(fmt::format("Invalid memfn declaration ({})", searchResult.error()));
    }
    std::span<const Token*> exceptTokens;
    if (classNameAndDoubleColonTokens) {
        exceptTokens = std::span<const Token*>(*classNameAndDoubleColonTokens);
    }
    return JoinTokensWithoutComments(
               std::span<const Token>(tokens0.data(), tokens.data() + openingBraceIdx),
               exceptTokens)
         + ';';
}

enum class StructOrClass { struct_, class_ };
template<>
struct enum_traits<StructOrClass> {
    using enum StructOrClass;
    static constexpr std::array<StructOrClass, 2> elements{struct_, class_};
    static constexpr std::array<std::string_view, elements.size()> names{"struct", "class"};
};
std::expected<std::string, std::string> ExtractStructOrClassDeclaration(
    StructOrClass structOrClass,
    std::string_view name,
    std::span<libtokenizer::Token> tokensForSearchClass) {
    using libtokenizer::Token;
    using libtokenizer::TokenType;
    auto assertValid = [](size_t x) {
        CHECK(x != SIZE_T_MAX);
        return x;
    };
    auto structOrClassName = enum_name(structOrClass);
    for (size_t classIdx = SIZE_T_MAX;;
         tokensForSearchClass = tokensForSearchClass.subspan(assertValid(classIdx) + 1),
                classIdx = SIZE_T_MAX) {
        auto searchResult =
            std::move(
                TokenSearch(tokensForSearchClass).Find(TokenType::kw, structOrClassName, &classIdx))
                .FinishSearch();
        if (!searchResult) {
            return std::unexpected(
                fmt::format("`{}` not found: {}", structOrClassName, searchResult.error()));
        }
        assert(classIdx != SIZE_T_MAX);
        auto tokensForSearchOpenBracket = tokensForSearchClass.subspan(classIdx + 1);
        // Get past the optional attributes.
        for (size_t startBracketIdx = SIZE_T_MAX;;
             tokensForSearchOpenBracket = tokensForSearchOpenBracket.subspan(startBracketIdx + 1)) {
            size_t endBracketIdx = SIZE_T_MAX;
            searchResult = std::move(TokenSearch(tokensForSearchOpenBracket)
                                         .Eat(TokenType::tok, "[", &startBracketIdx)
                                         .Eat(TokenType::tok, "]", &endBracketIdx))
                               .FinishSearch();
            if (searchResult) {
                // Both [ and ] found.
                assert(startBracketIdx != SIZE_T_MAX);
                assert(endBracketIdx != SIZE_T_MAX);
                // Continue after ]
                continue;
            } else {
                if (startBracketIdx != SIZE_T_MAX) {
                    // [ was found without ], error.
                    assert(endBracketIdx == SIZE_T_MAX);
                    return std::unexpected("Mismatched [ and ]");
                }
                // No brackets found, continue with name.
                break;
            }
        }
        auto tokenForSearchName = tokensForSearchOpenBracket;
        size_t nameIdx = SIZE_T_MAX;
        auto search = TokenSearch(tokenForSearchName).Eat(TokenType::id, name, &nameIdx);
        if (nameIdx == SIZE_MAX) {
            continue;
        }
        size_t colonIdx = SIZE_T_MAX;
        search.EatOptional(TokenType::kw, "final").EatOptional(TokenType::tok, ":", &colonIdx);
        size_t openingBraceIdx = SIZE_T_MAX;
        if (colonIdx == SIZE_T_MAX) {
            search.Eat(TokenType::tok, "{", &openingBraceIdx);
        } else {
            search.Find(TokenType::tok, "{", &openingBraceIdx);
        }
        if (openingBraceIdx == SIZE_T_MAX) {
            return std::unexpected(fmt::format("{} declaration not found.", structOrClassName));
        }
        size_t closingBraceIdx = SIZE_T_MAX;
        search.Eat(TokenType::tok, "}", &closingBraceIdx);
        if (closingBraceIdx == SIZE_T_MAX) {
            return std::unexpected("`}` not found.");
        }
        size_t closingSemicolonIdx = SIZE_T_MAX;
        search.Eat(TokenType::tok, ";", &closingSemicolonIdx);
        if (closingSemicolonIdx == SIZE_T_MAX) {
            return std::unexpected("Closing `;` not found.");
        }
        break;
    }
    return fmt::format("{} {};", structOrClassName, name);
}

struct ParsePreprocessedSourceResult {
    std::optional<Visibility> visibility;
    EntityDependentProperties::V dependentProps;
};
std::expected<ParsePreprocessedSourceResult, std::string> ParsePreprocessedSource(
    std::string_view name, const PreprocessedSource& pps, std::string_view parentDirName) {
    struct Collector {
        std::optional<Visibility> visibility;
        std::optional<EntityKind> entityKind;
        std::vector<std::string> fdneeds, needs, defneeds;
    } c;
    for (auto& sc : pps.specialComments) {
        if (auto keyword = enum_from_name<SpecialCommentKeyword>(sc.keyword)) {
            auto addToNeeds = [&sc](std::vector<std::string>& v) {
                v.reserve(v.size() + sc.list.size());
                for (auto& sv : sc.list) {
                    v.emplace_back(sv);
                }
            };
            switch (*keyword) {
                case SpecialCommentKeyword::fdneeds:
                    addToNeeds(c.fdneeds);
                    break;
                case SpecialCommentKeyword::needs:
                    addToNeeds(c.needs);
                    break;
                case SpecialCommentKeyword::defneeds:
                    addToNeeds(c.defneeds);
                    break;
                case SpecialCommentKeyword::visibility:
                    if (c.visibility) {
                        return std::unexpected(fmt::format("Duplicated `#visibility`, first value: "
                                                           "{}, second value: {}",
                                                           enum_name<Visibility>(*c.visibility),
                                                           fmt::join(sc.list, ", ")));
                    }
                    if (sc.list.size() != 1) {
                        return std::unexpected(
                            fmt::format("`#visibility` need a single value, got: {}",
                                        fmt::join(sc.list, ", ")));
                    }
                    c.visibility = enum_from_name<Visibility>(sc.list.front());
                    if (!c.visibility) {
                        return std::unexpected(
                            fmt::format("Invalid `#visibility` value: {}", sc.list.front()));
                    }
                    break;
            }
        } else if (auto entityKind = enum_from_name<EntityKind>(sc.keyword)) {
            if (!sc.list.empty()) {
                return std::unexpected(fmt::format(
                    "Unexpected list after `{}`: {}", sc.keyword, fmt::join(sc.list, ", ")));
            }
            if (c.entityKind) {
                return std::unexpected(fmt::format("Duplicated kind, first: {}, then: {}",
                                                   enum_name(*c.entityKind),
                                                   enum_name(*entityKind)));
            }
            c.entityKind = *entityKind;
        } else {
            return std::unexpected(fmt::format("Invalid keyword: {}", sc.keyword));
        }
    }
    if (!c.entityKind) {
        return std::unexpected(fmt::format("Missing `#<entity>` ({})",
                                           fmt::join(enum_traits<EntityKind>::names, ", ")));
    }
    //
    std::ranges::sort(c.fdneeds);
    std::ranges::sort(c.needs);
    std::ranges::sort(c.defneeds);
    CHECK(!pps.specialComments.empty());
    // Find the token corresponding to the first specialComment.
    auto firstSpecialCommentKeyword = pps.specialComments.front().keyword;

    auto it = std::ranges::lower_bound(
        pps.tokens.tokens, firstSpecialCommentKeyword.data(), {}, [](const libtokenizer::Token& t) {
            return t.sourceValue.data();
        });
    CHECK(it != pps.tokens.tokens.end()
          && data_plus_size(firstSpecialCommentKeyword) <= data_plus_size(it->sourceValue));

    auto firstSpecialCommentTokenIdx = it - pps.tokens.tokens.begin();

    using libtokenizer::Token;
    using libtokenizer::TokenType;
    auto tokensFromFirstSpecialComment = std::span<const Token>(
        pps.tokens.tokens.begin() + firstSpecialCommentTokenIdx, pps.tokens.tokens.end());

    struct {
        bool fdneeds{}, needs{}, defneeds{};
    } allowed;
    auto checkNeeds = [&allowed, &c]() -> std::expected<std::monostate, std::string> {
        if (!allowed.fdneeds && !c.fdneeds.empty()) {
            return std::unexpected(fmt::format("`{}` entities can't have fdneeds, here we have: {}",
                                               enum_name(*c.entityKind),
                                               fmt::join(c.fdneeds, ", ")));
        }
        if (!allowed.needs && !c.needs.empty()) {
            return std::unexpected(fmt::format("`{}` entities can't have needs, here we have: {}",
                                               enum_name(*c.entityKind),
                                               fmt::join(c.needs, ", ")));
        }
        if (!allowed.defneeds && !c.defneeds.empty()) {
            return std::unexpected(
                fmt::format("`{}` entities can't have defneeds, here we have: {}",
                            enum_name(*c.entityKind),
                            fmt::join(c.defneeds, ", ")));
        }
        return {};
    };
    std::optional<EntityDependentProperties::V> dependentProps;
    switch (*c.entityKind) {
        case EntityKind::enum_: {
            allowed.fdneeds = true;
            allowed.needs = true;
            TRY(checkNeeds());
            // Expected: enum ... name ... { ... } ;
            size_t enumIdx = SIZE_T_MAX;
            size_t openingBraceIdx = SIZE_T_MAX;
            auto searchResult = std::move(TokenSearch(tokensFromFirstSpecialComment)
                                              .Eat(TokenType::kw, "enum", &enumIdx)
                                              .Find(TokenType::id, name)
                                              .Find(TokenType::tok, "{", &openingBraceIdx)
                                              .Eat(TokenType::tok, "}")
                                              .Eat(TokenType::tok, ";")
                                              .AssertEnd())
                                    .FinishSearch();
            if (!searchResult.has_value()) {
                return std::unexpected(
                    fmt::format("Invalid enum declaration ({})", searchResult.error()));
            }
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(EntityKind::enum_)>,
                EntityDependentProperties::Enum{
                    .opaqueEnumDeclaration =
                        JoinTokensWithoutComments(tokensFromFirstSpecialComment.subspan(
                            enumIdx, openingBraceIdx - enumIdx))
                        + ";",
                    .opaqueEnumDeclarationNeeds = std::move(c.fdneeds),
                    .declarationNeeds = std::move(c.needs)});
        } break;
        case EntityKind::fn: {
            allowed.needs = true;
            allowed.defneeds = true;
            TRY(checkNeeds());
            // Expected: ... name ( ... ) ... { ... }
            TRY_ASSIGN(
                declaration,
                ExtractFunctionDeclaration(std::nullopt, name, tokensFromFirstSpecialComment));
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(EntityKind::fn)>,
                EntityDependentProperties::Fn{.declaration = std::move(declaration),
                                              .declarationNeeds = std::move(c.needs),
                                              .definitionNeeds = std::move(c.defneeds)});
        } break;
        case EntityKind::struct_: {
            allowed.fdneeds = true;
            allowed.needs = true;
            TRY(checkNeeds());

            // Expected: ... `struct` attributes-opt name (`:` ...)? `{` ... `}` `;`
            // TRY_ASSIGN(declaration, ExtractClassOrStructDeclaration(StructOrClass::struct_, name,
            // tokensFromFirstSpecialComment));
            auto declaration = fmt::format("struct {};", name);
            dependentProps.emplace(std::in_place_index<std::to_underlying(EntityKind::struct_)>,
                                   EntityDependentProperties::StructOrClass{
                                       .forwardDeclaration = std::move(declaration),
                                       .forwardDeclarationNeeds = std::move(c.fdneeds),
                                       .declarationNeeds = std::move(c.needs)});

        } break;
        case EntityKind::class_: {
            allowed.fdneeds = true;
            allowed.needs = true;
            TRY(checkNeeds());
            // Expected: ... `class` attributes-opt name (`:` ...)? `{` ... `}` `;`
            // TRY_ASSIGN(declaration, ExtractClassOrStructDeclaration(StructOrClass::class_, name,
            // tokensFromFirstSpecialComment));
            auto declaration = fmt::format("class {};", name);
            dependentProps.emplace(std::in_place_index<std::to_underlying(EntityKind::class_)>,
                                   EntityDependentProperties::StructOrClass{
                                       .forwardDeclaration = std::move(declaration),
                                       .forwardDeclarationNeeds = std::move(c.fdneeds),
                                       .declarationNeeds = std::move(c.needs)});
        } break;
        case EntityKind::header: {
            allowed.needs = true;
            TRY(checkNeeds());
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(EntityKind::header)>,
                EntityDependentProperties::Header{.declarationNeeds = std::move(c.needs)});
        } break;
        case EntityKind::memfn: {
            allowed.needs = true;
            allowed.defneeds = true;
            TRY(checkNeeds());
            // Expected: ... classname :: name ( ... ) ... { ... }
            TRY_ASSIGN(
                declaration,
                ExtractFunctionDeclaration(parentDirName, name, tokensFromFirstSpecialComment));
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(EntityKind::memfn)>,
                EntityDependentProperties::MemFn{.declaration = std::move(declaration),
                                                 .declarationNeeds = std::move(c.needs),
                                                 .definitionNeeds = std::move(c.defneeds)});
        } break;
    }
    CHECK(dependentProps);

    return ParsePreprocessedSourceResult{.visibility = c.visibility,
                                         .dependentProps = std::move(*dependentProps)};
}

std::expected<std::monostate, std::string> ProcessSource(
    const std::filesystem::path& sourcePath,
    const ProgramOptions& programOptions,
    flat_hash_map<std::string, Entity>& entities) {
    (void)entities;  // TODO
    TRY_ASSIGN_OR_UNEXPECTED(sourceContent, ReadFile(sourcePath), "Can't open file for reading");
    TRY_ASSIGN(pps, PreprocessSource(sourceContent));

    if (pps.specialComments.empty()) {
        if (programOptions.verbose) {
            fmt::print(stderr, "Ignoring file without special comments: {}\n", sourcePath);
        }
        return {};
    }

    std::string name = path_to_string(sourcePath.stem());

    if (!sourcePath.has_parent_path()) {
        return std::unexpected(fmt::format("File {} has no parent path.", sourcePath));
    }

    std::string parentDirName = path_to_string(sourcePath.parent_path().stem());

    TRY_ASSIGN(ep, ParsePreprocessedSource(name, pps, parentDirName));

    auto entity = Entity{.name = name,
                         .path = sourcePath,
                         .visibility = ep.visibility.value_or(Entity::k_defaultVisibility),
                         .dependentProps = std::move(ep.dependentProps)};
    auto entityKind = entity.GetEntityKind();
    auto itb = entities.insert(std::make_pair(name, std::move(entity)));
    if (!itb.second) {
        return std::unexpected(fmt::format("Duplicate name: `{}`, first {} then {}.",
                                           name,
                                           enum_name(itb.first->second.GetEntityKind()),
                                           enum_name(entityKind)));
    }

#if 0
    std::string name;
    auto entity = Entity{.name = name, .path = sf, .props = std::move(ep)};
    auto itb = entities.insert(std::make_pair(std::move(name), std::move(entity)));
    if (!itb.second) {
        return std::unexpected(fmt::format("Duplicate name: `{}`, first {} then {}.",
                                           entity.name,
                                           enum_name(itb.first->second.GetEntityKind()),
                                           enum_name(entity.GetEntityKind())));
    }
#endif
    return std::monostate();
}

struct path_hash {
    size_t operator()(const fs::path p) const {
        return hash_value(p);
    }
};

struct GeneratedFileWriter {
    explicit GeneratedFileWriter(fs::path outputDir_)
        : outputDir(std::move(outputDir_)) {
        std::error_code ec;
        fs::create_directories(outputDir, ec);
        LOG_IF(QFATAL, ec) << fmt::format("Can't create output directory `{}`.", outputDir);

        // Enumerate current files in the output directory.
        auto dit = fs::recursive_directory_iterator(outputDir, fs::directory_options::none, ec);
        LOG_IF(QFATAL, ec) << fmt::format("Can't get listing of output directory `{}`.", outputDir);
        for (auto const& de : dit) {
            auto d = de.is_directory(ec);
            LOG_IF(FATAL, ec) << fmt::format("Can't query if directory entry is a directory: {}",
                                             de.path());
            if (d) {
                remainingExistingDirs.insert(de.path());
            } else {
                remainingExistingFiles.insert(de.path());
            }
        }
    }
    ~GeneratedFileWriter() {
        assert(remainingExistingFiles.empty() && remainingExistingDirs.empty());
        RemoveRemainingExistingFilesAndDirs();
    }
    void Write(const fs::path& relPath, std::string_view content) {
        auto relDir = relPath;
        bool firstParent = true;
        for (; relDir.has_parent_path();) {
            relDir = relDir.parent_path();
            auto absDir = outputDir / relDir;
            if (remainingExistingDirs.erase(absDir) == 0 && firstParent) {
                std::error_code ec;
                [[maybe_unused]] bool created = fs::create_directories(absDir, ec);
                LOG_IF(FATAL, ec) << fmt::format("Couldn't create directories: {}", absDir);
            }
            firstParent = false;
        }
        bool create_directories(const std::filesystem::path& p, std::error_code& ec);

        auto path = outputDir / relPath;
        remainingExistingFiles.erase(path);
        currentFiles.push_back(path);
        auto existingContent = ReadFile(path);
        if (existingContent != content) {
            LOG_IF(FATAL, !WriteFile(path, content)) << fmt::format("Couldn't write {}.", path);
        }  // Else no change, no need to write.
    }
    void RemoveRemainingExistingFilesAndDirs() {
        std::error_code ec;
        for (auto& f : remainingExistingFiles) {
            fs::remove(f, ec);  // Ignore if couldn't remove it.
        }
        remainingExistingFiles.clear();
        for (auto& d : remainingExistingDirs) {
            fs::remove_all(d, ec);
        }
        remainingExistingDirs.clear();  // Ignore if couldn't remove it.
    }
    // All paths should be absolute.
    fs::path outputDir;
    flat_hash_set<fs::path, path_hash> remainingExistingFiles;
    std::vector<fs::path> currentFiles;
    flat_hash_set<fs::path, path_hash> remainingExistingDirs;
};

int main(int argc, char* argv[]) {
    absl::InitializeLog();

    CLI::App app("C++ boilerplate generator", "NMT");

    ProgramOptions args;

    app.add_option(
        "-s,--sources",
        args.sources,
        fmt::format("List of source files ({}) or files containing a list of source files ({}, one "
                    "path on each line) or any other files (they'll be ignored)",
                    fmt::join(k_validSourceExtensions, ", "),
                    fmt::join(k_fileListExtensions, ", ")));
    app.add_option("-o,--output-dir",
                   args.outputDir,
                   "Output directory, will be created or content erased/updated, as needed")
        ->required();
    app.add_flag("-v,--verbose", args.verbose, "Print more diagnostics");

    CLI11_PARSE(app, argc, argv);

    auto sourcesOr = ResolveArgsSources(args.sources, args.verbose);
    if (!sourcesOr) {
        return EXIT_FAILURE;
    }
    auto sources = std::move(*sourcesOr);

    fmt::print("sources: {}\n", fmt::join(sources, ", "));

    flat_hash_map<std::string, Entity> entities;

    for (auto& sf : sources) {
        auto r = ProcessSource(sf, args, entities);
        if (!r) {
            fmt::print(stderr, "Failed to process {}: {}\n", sf, r.error());
            return EXIT_FAILURE;
        }
    }

    GeneratedFileWriter gfw(args.outputDir);

    // Generate empty header.
    gfw.Write(k_emptyHeaderFilename,
              fmt::format("{}\n{}\n", k_autogeneratedWarningLine, k_IntentionallyEmptyLineWarning));

    struct Includes {
        std::vector<std::string> generateds, locals, externalsInDirs, externalWithExtension,
            externalsWithoutExtension, forwardDeclarations;
        void addHeader(std::string_view s) {
            assert(!s.empty());
            std::vector<std::string>* v{};
            if (s[0] == '"') {
                v = &locals;
            } else if (s.find('/') != std::string_view::npos) {
                v = &externalsInDirs;
            } else if (s.find('.') != std::string_view::npos) {
                v = &externalWithExtension;
            } else {
                v = &externalsWithoutExtension;
            }
            v->push_back(std::string(s));
        };
        std::string render() {
            std::string content;
            auto addHeaders = [&content](std::vector<std::string>& v) {
                if (!v.empty()) {
                    if (!content.empty()) {
                        content += "\n";
                    }
                    std::sort(v.begin(), v.end());
                    for (auto& s : v) {
                        content += fmt::format("#include {}\n", s);
                    }
                }
            };
            addHeaders(generateds);
            addHeaders(locals);
            addHeaders(externalsInDirs);
            addHeaders(externalWithExtension);
            addHeaders(externalsWithoutExtension);
            if (!forwardDeclarations.empty()) {
                if (!content.empty()) {
                    content += "\n";
                }
                std::sort(forwardDeclarations.begin(), forwardDeclarations.end());
                for (auto& s : forwardDeclarations) {
                    content += fmt::format("{}\n", s);
                }
            }
            return content;
        }
        void addNeedsAsHeaders(const flat_hash_map<std::string, Entity>& entities,
                               const Entity& e,
                               const std::vector<std::string>& needs,
                               const fs::path& outputDir) {
            std::vector<std::string> additionalNeeds;
            const std::vector<std::string>* currentNeeds = &needs;
            while (!currentNeeds->empty()) {
                additionalNeeds = addNeedsAsHeadersCore(entities, e, *currentNeeds, outputDir);
                // TODO: prevent infinite loop because of some circular dependency.
                currentNeeds = &additionalNeeds;
            }
        }
        // Return additional needs coming from the enums which have needs for their
        // opaque-enum-declaration.
        std::vector<std::string> addNeedsAsHeadersCore(
            const flat_hash_map<std::string, Entity>& entities,
            const Entity& e,
            const std::vector<std::string>& needs,
            const fs::path& outputDir) {
            std::vector<std::string> additionalNeeds;
            for (auto& need : needs) {
                LOG_IF(FATAL, need.empty()) << "Empty need name.";
                if (need[0] == '<' || need[0] == '"') {
                    addHeader(need);
                } else {
                    std::string_view needName = need;
                    const bool refOnly = needName.back() == '*';
                    if (refOnly) {
                        needName.remove_suffix(1);
                    }
                    LOG_IF(FATAL, needName == e.name)
                        << fmt::format("Entity `{}` can't include itself.", e.name);
                    auto it = entities.find(std::string(needName));  // TODO(optimize): could use
                                                                     // heterogenous lookup.
                    LOG_IF(FATAL, it == entities.end()) << fmt::format(
                        "Entity `{}` needs `{}` but it's missing.", e.name, needName);
                    auto& ne = it->second;
                    if (refOnly) {
                        auto* v = ne.ForwardDeclarationNeedsOrNull();
                        LOG_IF(FATAL, v == nullptr)
                            << fmt::format("Entity `{}` needs `{}` but it's a {} and can't "
                                           "be forward declared.",
                                           e.name,
                                           need,
                                           enum_name(ne.GetEntityKind()));
                        additionalNeeds.insert(additionalNeeds.end(), BEGIN_END(*v));
                        forwardDeclarations.push_back(fmt::format("{}", ne.ForwardDeclaration()));
                    } else {
                        auto path = outputDir / ne.HeaderPath();
                        generateds.push_back(fmt::format("\"{}\"", path));
                    }
                }
            }
            return additionalNeeds;
        }
    };

    flat_hash_map<std::string, std::vector<std::string>> membersOfClasses;
    for (auto& [_, e] : entities) {
        if (e.GetEntityKind() == EntityKind::memfn) {
            membersOfClasses[e.MemFnClassName()].push_back(e.name);
        }
    }

    for (auto& [_, e] : entities) {
        if (e.GetEntityKind() != EntityKind::memfn) {
            std::string headerContent =
                fmt::format("{}\n#pragma once\n", k_autogeneratedWarningLine);
            {
                Includes includes;
                auto addNeedsAsHeaders =
                    [&includes, &args, &entities, &e](const std::vector<std::string>& needs) {
                        includes.addNeedsAsHeaders(entities, e, needs, args.outputDir);
                    };
                switch (e.GetEntityKind()) {
                    case EntityKind::enum_:
                        addNeedsAsHeaders(
                            std::get<EntityDependentProperties::Enum>(e.dependentProps)
                                .opaqueEnumDeclarationNeeds);
                        addNeedsAsHeaders(
                            std::get<EntityDependentProperties::Enum>(e.dependentProps)
                                .declarationNeeds);
                        break;
                    case EntityKind::fn:
                        addNeedsAsHeaders(std::get<EntityDependentProperties::Fn>(e.dependentProps)
                                              .declarationNeeds);
                        break;
                    case EntityKind::struct_: {
                        addNeedsAsHeaders(
                            std::get<std::to_underlying(EntityKind::struct_)>(e.dependentProps)
                                .forwardDeclarationNeeds);
                        addNeedsAsHeaders(
                            std::get<std::to_underlying(EntityKind::struct_)>(e.dependentProps)
                                .declarationNeeds);
                        auto it = membersOfClasses.find(e.name);
                        if (it != membersOfClasses.end()) {
                            for (auto& mf : it->second) {
                                auto& me = entities.at(mf);
                                assert(me.GetEntityKind() == EntityKind::memfn);
                                auto& dp = std::get<std::to_underlying(EntityKind::memfn)>(
                                    me.dependentProps);
                                addNeedsAsHeaders(dp.declarationNeeds);
                            }
                        }
                    } break;
                    case EntityKind::class_: {
                        addNeedsAsHeaders(
                            std::get<std::to_underlying(EntityKind::class_)>(e.dependentProps)
                                .forwardDeclarationNeeds);
                        addNeedsAsHeaders(
                            std::get<std::to_underlying(EntityKind::class_)>(e.dependentProps)
                                .declarationNeeds);
                        auto it = membersOfClasses.find(e.name);
                        if (it != membersOfClasses.end()) {
                            for (auto& mf : it->second) {
                                auto& me = entities.at(mf);
                                assert(me.GetEntityKind() == EntityKind::memfn);
                                auto& dp = std::get<std::to_underlying(EntityKind::memfn)>(
                                    me.dependentProps);
                                addNeedsAsHeaders(dp.declarationNeeds);
                            }
                        }
                    } break;
                    case EntityKind::header:
                        addNeedsAsHeaders(
                            std::get<EntityDependentProperties::Header>(e.dependentProps)
                                .declarationNeeds);
                        break;
                    case EntityKind::memfn:
                        // memfn's declaration needs go to the class/struct header.
                        break;
                }
                auto renderedHeaders = includes.render();
                if (!renderedHeaders.empty()) {
                    headerContent += fmt::format("\n{}", renderedHeaders);
                }
            }
            headerContent += "\n";
            switch (e.GetEntityKind()) {
                case EntityKind::enum_:
                case EntityKind::header:
                    headerContent += fmt::format("#include \"{}\"\n", e.path);
                    break;
                case EntityKind::fn:
                    headerContent += fmt::format(
                        "{}\n",
                        std::get<EntityDependentProperties::Fn>(e.dependentProps).declaration);
                    break;
                case EntityKind::struct_:
                case EntityKind::class_: {
                    // TODO (not here) determine source files' relative dires and
                    // generate output files according to relative dirs.
                    // TODO (not here): check how fmt::format formats paths on Windows (unicode)
                    // TODO: for k_nmtIncludeMemberDeclarationsMacro consider using either R""
                    // literals or generic_path, because of the slashes and other characters. Also
                    // need to verify how unicode paths work.
                    auto it = membersOfClasses.find(e.name);
                    fs::path memberDeclarationsPath;
                    if (it == membersOfClasses.end()) {
                        memberDeclarationsPath = path_from_string(k_emptyHeaderFilename);
                    } else {
                        memberDeclarationsPath = e.MemberDeclarationsPath();
                        std::string mdContent = fmt::format("{}\n", k_autogeneratedWarningLine);
                        for (auto& mf : it->second) {
                            auto& mfe = entities.at(mf);
                            auto& dp =
                                std::get<std::to_underlying(EntityKind::memfn)>(mfe.dependentProps);
                            mdContent += fmt::format("{}\n", dp.declaration);
                        }
                        gfw.Write(memberDeclarationsPath, mdContent);
                    }
                    headerContent += fmt::format("#define {} \"{}\"\n#include \"{}\"\n#undef {}\n",
                                                 k_nmtIncludeMemberDeclarationsMacro,
                                                 args.outputDir / memberDeclarationsPath,
                                                 e.path,
                                                 k_nmtIncludeMemberDeclarationsMacro);
                } break;
                case EntityKind::memfn:
                    // memfn declaration goes to the member function declaration file which gets
                    // injected with a macro.
                    CHECK(false);
                    break;
            }

            fmt::print("Processed {} from {}\n", e.name, e.path);
            gfw.Write(e.HeaderPath(), headerContent);
        }
        std::string cppContent;
        switch_variant(
            e.dependentProps,
            [&args, &entities, &cppContent, &e](const EntityDependentProperties::Fn& dp) {
                cppContent += fmt::format("{}\n#include \"{}\"\n",
                                          k_autogeneratedWarningLine,
                                          args.outputDir / e.HeaderPath());
                Includes includes;
                includes.addNeedsAsHeaders(entities, e, dp.definitionNeeds, args.outputDir);
                auto renderedHeaders = includes.render();
                if (!renderedHeaders.empty()) {
                    cppContent += fmt::format("\n{}", renderedHeaders);
                }
                cppContent += fmt::format("\n#include \"{}\"\n", e.path);
            },
            [&args, &entities, &cppContent, &e](const EntityDependentProperties::MemFn& dp) {
                cppContent +=
                    fmt::format("{}\n#include \"{}\"\n",
                                k_autogeneratedWarningLine,
                                args.outputDir / entities.at(e.MemFnClassName()).HeaderPath());
                Includes includes;
                includes.addNeedsAsHeaders(entities, e, dp.definitionNeeds, args.outputDir);
                auto renderedHeaders = includes.render();
                if (!renderedHeaders.empty()) {
                    cppContent += fmt::format("\n{}", renderedHeaders);
                }
                cppContent += fmt::format("\n#include \"{}\"\n", e.path);
            },
            [&args, &cppContent, headerPath = e.HeaderPath()](const auto&) {
                cppContent += fmt::format("{}\n#include \"{}\"\n",
                                          k_autogeneratedWarningLine,
                                          args.outputDir / headerPath);
            });
        gfw.Write(e.DefinitionPath(), cppContent);
    }  // for a: entities
    std::ranges::sort(gfw.currentFiles);
    std::string fileListContent;
    for (auto& c : gfw.currentFiles) {
        fileListContent += fmt::format("{}\n", c);
    }
    gfw.Write(k_fileListFilename, fileListContent);
    gfw.RemoveRemainingExistingFilesAndDirs();
    return EXIT_SUCCESS;
}

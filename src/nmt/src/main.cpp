#include "pch.h"

#include "parse.h"

#include "PreprocessSource.h"
#include "ReadFile.h"
#include "ReadFileAsLines.h"
#include "WriteFile.h"
#include "data.h"
#include "fn_CompactSpaces.h"
#include "fn_ExtractIdentifier.h"
#include "fn_ParseEnum.h"
#include "fn_ParseFn.h"
#include "fn_ParseStructClass.h"
// #include "fn_fs_exists_total.h"
#include "TokenSearch.h"
#include "sc_Args.h"

namespace fs = std::filesystem;

constexpr std::string_view k_autogeneratedWarningLine = "// AUTOGENERATED FILE, DO NOT EDIT!";
const std::set<fs::path> k_validSourceExtensions = {".h", ".hpp", ".hxx"};
const std::set<fs::path> k_fileListExtensions = {".txt", ".lst"};
constexpr std::string_view k_nmtIncludeMemberDeclarationsMacro = "NMT_MEMBER_DECLARATIONS";
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

struct ParsePreprocessedSourceResult {
    std::optional<EntityKind> entityKind;
    std::vector<std::string> fdneeds, needs, defneeds;
    std::optional<Visibility> visibility;
};

std::expected<ParsePreprocessedSourceResult, std::string> ParsePreprocessedSource(
    std::string_view name, const PreprocessedSource& pps, std::string_view parentDirName) {
    ParsePreprocessedSourceResult result;
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
                    addToNeeds(result.fdneeds);
                    break;
                case SpecialCommentKeyword::needs:
                    addToNeeds(result.needs);
                    break;
                case SpecialCommentKeyword::defneeds:
                    addToNeeds(result.defneeds);
                    break;
                case SpecialCommentKeyword::visibility:
                    if (result.visibility) {
                        return std::unexpected(
                            fmt::format("Duplicated `#visibility`, first value: "
                                        "{}, second value: {}",
                                        enum_name<Visibility>(*result.visibility),
                                        fmt::join(sc.list, ", ")));
                    }
                    if (sc.list.size() != 1) {
                        return std::unexpected(
                            fmt::format("`#visibility` need a single value, got: {}",
                                        fmt::join(sc.list, ", ")));
                    }
                    result.visibility = enum_from_name<Visibility>(sc.list.front());
                    if (!result.visibility) {
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
            if (result.entityKind) {
                return std::unexpected(fmt::format("Duplicated kind, first: {}, then: {}",
                                                   enum_name(*result.entityKind),
                                                   enum_name(*entityKind)));
            }
            result.entityKind = *entityKind;
        } else {
            return std::unexpected(fmt::format("Invalid keyword: {}", sc.keyword));
        }
    }
    if (!result.entityKind) {
        return std::unexpected(fmt::format("Missing `#<entity>` ({})",
                                           fmt::join(enum_traits<EntityKind>::names, ", ")));
    }
#if 0
    //
    // Find the first nonCommentCode after the first specialComment.
    CHECK(!pps.specialComments.empty());
    auto& firstSpecialComment = pps.specialComments.begin()->keyword;
    auto it = pps.nonCommentCode.begin();
    while (it != pps.nonCommentCode.end() && it->data() < firstSpecialComment.data()) {
        ++it;
    }
    if (it == pps.nonCommentCode.end()) {
        // All nonCommentCode is before the first specialComment.
        return std::unexpected(
            "All code is before the first special comment. Move at leat the first special "
            "comment before the main entity in the file.\n");
    }
    // Assemble all the remaining code into a single string.
    std::string code;
    for (; it != pps.nonCommentCode.end(); ++it) {
        code += *it;
        code += ' ';
    }
    auto decl = trim(code);
    if (decl.empty()) {
        return std::unexpected(
            fmt::format("No code found for `{}` entity.", enum_name(*result.entityKind)));
    }
    if (decl.back() != ';') {
        return std::unexpected(fmt::format("Missing trailing semicolon at end of `{}` entity.",
                                           enum_name(*result.entityKind)));
    }
#endif
    //
    std::ranges::sort(result.fdneeds);
    std::ranges::sort(result.needs);
    std::ranges::sort(result.defneeds);
    struct {
        bool fdneeds{}, needs{}, defneeds{};
    } allowed;
    std::optional<EntityDependentProperties::V> dependentProps;
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
    switch (*result.entityKind) {
        case EntityKind::enum_: {
            allowed.fdneeds = true;
            allowed.needs = true;
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
            auto opaqueEnumDeclaration =
                std::string_view(tokensFromFirstSpecialComment[enumIdx].sourceValue.data(),
                                 tokensFromFirstSpecialComment[openingBraceIdx].sourceValue.data());
            dependentProps.emplace(std::in_place_index<std::to_underlying(EntityKind::enum_)>,
                                   EntityDependentProperties::Enum{
                                       .opaqueEnumDeclaration = std::string(opaqueEnumDeclaration),
                                       .opaqueEnumDeclarationNeeds = std::move(result.fdneeds),
                                       .declarationNeeds = std::move(result.needs)});
        } break;
        case EntityKind::fn:
            allowed.needs = true;
            allowed.defneeds = true;
#if 0
            auto declUntilFirstOpeningBrace = TrimBeforeOpeningBrace(decl);
            if (declUntilFirstOpeningBrace.empty()) {
                return std::unexpected("No `{` found in function definition.");
            }
            dependentProps.emplace(std::in_place_index<std::to_underlying(EntityKind::fn)>,
                                   EntityDependentProperties::Fn{
                                       .declaration = std::string(declUntilFirstOpeningBrace),
                                       .declarationNeeds = std::move(result.needs),
                                       .definitionNeeds = std::move(result.defneeds)});
#endif
            break;
        case EntityKind::struct_:
            allowed.fdneeds = true;
            allowed.needs = true;
#if 0
            dependentProps.emplace(std::in_place_index<std::to_underlying(struct_)>,
                                   EntityDependentProperties::StructOrClass{
                                       .forwardDeclarationNeeds = std::move(result.fdneeds),
                                       .declarationNeeds = std::move(result.needs)});
#endif
            break;
        case EntityKind::class_: {
            allowed.fdneeds = true;
            allowed.needs = true;
            // Expected: ... `class` attributes-opt name (`:` ...)? `{` ... `}` `;`
            auto tokensForSearchClass = tokensFromFirstSpecialComment;
            auto assertValid = [](size_t x) {
                CHECK(x != SIZE_T_MAX);
                return x;
            };
            for (size_t classIdx = SIZE_T_MAX;;
                 tokensForSearchClass = tokensForSearchClass.subspan(assertValid(classIdx) + 1),
                        classIdx = SIZE_T_MAX) {
                auto searchResult =
                    std::move(
                        TokenSearch(tokensForSearchClass).Find(TokenType::kw, "class", &classIdx))
                        .FinishSearch();
                if (!searchResult) {
                    return std::unexpected(
                        fmt::format("`class` not found: {}", searchResult.error()));
                }
                assert(classIdx != SIZE_T_MAX);
                auto tokensForSearchOpenBracket = tokensForSearchClass.subspan(classIdx + 1);
                // Get past the optional attributes.
                for (size_t startBracketIdx = SIZE_T_MAX;;
                     tokensForSearchOpenBracket =
                         tokensForSearchOpenBracket.subspan(startBracketIdx + 1)) {
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
                search.EatOptional(TokenType::kw, "final")
                    .EatOptional(TokenType::tok, ":", &colonIdx);
                size_t openingBraceIdx = SIZE_T_MAX;
                if (colonIdx == SIZE_T_MAX) {
                    search.Eat(TokenType::tok, "{", &openingBraceIdx);
                } else {
                    search.Find(TokenType::tok, "{", &openingBraceIdx);
                }
                if (openingBraceIdx == SIZE_T_MAX) {
                    return std::unexpected("class declaration not found.");
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

            dependentProps.emplace(std::in_place_index<std::to_underlying(EntityKind::class_)>,
                                   EntityDependentProperties::StructOrClass{
                                       .forwardDeclaration = fmt::format("class {};", name),
                                       .forwardDeclarationNeeds = std::move(result.fdneeds),
                                       .declarationNeeds = std::move(result.needs)});
        } break;
        case EntityKind::using_: {
            allowed.needs = true;
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(using_)>,
                EntityDependentProperties::Using{.declaration = CompactSpaces(decl),
                                                 .declarationNeeds = std::move(result.needs)});
        } break;
        case EntityKind::inlvar:
            allowed.needs = true;
#if 0
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(inlvar)>,
                EntityDependentProperties::InlVar{.declarationNeeds = std::move(result.needs)});
#endif
            break;
        case EntityKind::memfn: {
            allowed.needs = true;
            allowed.defneeds = true;
            // Expected: ... classname :: name ( ... ) ... { ... }
            auto className = parentDirName;
            auto tokens = tokensFromFirstSpecialComment;
            size_t openingParenIdx = SIZE_T_MAX;
            for (;;) {
                size_t classNameIdx = SIZE_T_MAX;
                auto searchResult = std::move(TokenSearch(tokens)
                                                  .Find(TokenType::id, className, &classNameIdx)
                                                  .Eat(TokenType::tok, "::")
                                                  .Eat(TokenType::id, name)
                                                  .Eat(TokenType::tok, "(", &openingParenIdx))
                                        .FinishSearch();
                if (classNameIdx == SIZE_T_MAX) {
                    // Even the `className` token wasn't found, no need to
                    // backtrack.
                    return std::unexpected(fmt::format(
                        "Pattern <class-name>::<memfn-name> not found ({}::{})", className, name));
                }
                if (searchResult.has_value()) {
                    // The `(` was also found.
                    break;
                }
                // Backtract, start over from where the clasName was found.
                tokens = tokens.subspan(classNameIdx + 1);
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
                return std::unexpected(
                    fmt::format("Invalid memfn declaration ({})", searchResult.error()));
            }
            // Find the last noncomment before opening brace
            CHECK(openingBraceIdx > 0);
            size_t declarationBackIdx = openingBraceIdx - 1;
            while (declarationBackIdx > 0 && tokens[declarationBackIdx].IsComment()) {
                --declarationBackIdx;
            }
            CHECK(declarationBackIdx > 0);
            auto declaration =
                std::string_view(tokensFromFirstSpecialComment.front().sourceValue.data(),
                                 data_plus_size(tokens[declarationBackIdx].sourceValue));
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(EntityKind::memfn)>,
                EntityDependentProperties::MemFn{.declaration = std::string(declaration),
                                                 .declarationNeeds = std::move(result.needs),
                                                 .definitionNeeds = std::move(result.defneeds)});
#if 0
			auto declUntilFirstOpeningBrace = TrimBeforeOpeningBrace(decl);
			if (declUntilFirstOpeningBrace.empty()) {
				return std::unexpected("No `{` found in member function definition.");
			}
			// Also find ClassName::Function
#endif
        } break;
    }
    if (!allowed.fdneeds && !result.fdneeds.empty()) {
        return std::unexpected(fmt::format("`{}` entities can't have fdneeds, here we have: {}",
                                           enum_name(*result.entityKind),
                                           fmt::join(result.fdneeds, ", ")));
    }
    if (!allowed.needs && !result.needs.empty()) {
        return std::unexpected(fmt::format("`{}` entities can't have needs, here we have: {}",
                                           enum_name(*result.entityKind),
                                           fmt::join(result.needs, ", ")));
    }
    if (!allowed.defneeds && !result.defneeds.empty()) {
        return std::unexpected(fmt::format("`{}` entities can't have defneeds, here we have: {}",
                                           enum_name(*result.entityKind),
                                           fmt::join(result.defneeds, ", ")));
    }
    return result;
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
    (void)ep;  // TODO
#if 0
    auto f =
        [&ep, &sf, &entities](std::expected<std::pair<std::string, std::string>, std::string> edOr)
        -> std::expected<std::monostate, std::string> {
        if (!edOr) {
            return std::unexpected(fmt::format("Failed to parse {} in {}, reason: {}",
                                               enum_name(ep.GetEntityKind()),
                                               sf,
                                               edOr.error()));
        }
        switch_variant(
            ep.dependentProps,
            [&edOr](EntityDependentProperties::Enum& x) {
                x.opaqueEnumDeclaration = std::move(edOr->second);
            },
            [&edOr](EntityDependentProperties::Fn& x) {
                x.declaration = std::move(edOr->second);
            },
            [&edOr](EntityDependentProperties::StructOrClass& x) {
                x.forwardDeclaration = std::move(edOr->second);
            },
            [](const EntityDependentProperties::Using&) {
                LOG(FATAL) << "\"using\" entity got nonempty declaration from parse";
            },
            [](const EntityDependentProperties::InlVar&) {
                LOG(FATAL) << "\"inlvar\" entity got nonempty declaration from parse";
            });
        auto entity = Entity{.name = edOr->first, .path = sf, .props = std::move(ep)};
        auto itb = entities.insert(std::make_pair(std::move(edOr->first), std::move(entity)));
        if (!itb.second) {
            return std::unexpected(fmt::format("Duplicate name: `{}`, first {} then {}.",
                                               entity.name,
                                               enum_name(itb.first->second.GetEntityKind()),
                                               enum_name(entity.GetEntityKind())));
        }
        return std::monostate{};
    };

    switch (ep.GetEntityKind()) {
        case EntityKind::enum_:
            return f(ParseEnum(code));
            break;
        case EntityKind::fn:
            return f(ParseFunction(code));
            break;
        case EntityKind::struct_:
        case EntityKind::class_:
            return f(ParseStructClassDeclaration(code));
        case EntityKind::using_:
        case EntityKind::inlvar:
            return std::unexpected("Not implemented");
    }
#endif
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

    std::error_code ec;
    fs::create_directories(args.outputDir, ec);
    LOG_IF(QFATAL, ec) << fmt::format("Can't create output directory `{}`.", args.outputDir);

    auto dit = fs::directory_iterator(args.outputDir, fs::directory_options::none, ec);
    LOG_IF(QFATAL, ec) << fmt::format("Can't get listing of output directory `{}`.",
                                      args.outputDir);
    struct path_hash {
        size_t operator()(const fs::path p) const {
            return hash_value(p);
        }
    };
    flat_hash_set<fs::path, path_hash> remainingExistingFiles;
    for (auto const& de : dit) {
        auto ext = de.path().extension().string();
        if (ext == ".h" || ext == ".cpp") {
            remainingExistingFiles.insert(de.path());
        }
    }
    std::vector<fs::path> currentFiles;

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
                    content += s;
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
                    CHECK(false);  // TODO what's the next switch?
                    switch (ne.GetEntityKind()) {
                        case EntityKind::enum_:
                        case EntityKind::struct_:
                        case EntityKind::class_:
                            break;
                        case EntityKind::fn:
                        case EntityKind::using_:
                        case EntityKind::inlvar:
                        case EntityKind::memfn:
                            break;
                    }
                    if (refOnly) {
                        auto* v = ne.ForwardDeclarationNeedsOrNull();
                        LOG_IF(FATAL, v == nullptr)
                            << fmt::format("Entity `{}` needs `{}` but it's a {} and can't "
                                           "be forward declared.",
                                           e.name,
                                           need,
                                           enum_name(ne.GetEntityKind()));
                        additionalNeeds.insert(additionalNeeds.end(), BEGIN_END(*v));
                        forwardDeclarations.push_back(fmt::format("{};", ne.ForwardDeclaration()));
                    } else {
                        auto path = outputDir / path_from_string(ne.HeaderFilename());
                        generateds.push_back(fmt::format("\"{}\"", path));
                    }
                }
            }
            return additionalNeeds;
        }
    };

    for (auto& [_, e] : entities) {
        std::string headerContent = fmt::format("{}\n#pragma once\n", k_autogeneratedWarningLine);
        {
            Includes includes;
            auto addNeedsAsHeaders =
                [&includes, &args, &entities, &e](const std::vector<std::string>& needs) {
                    includes.addNeedsAsHeaders(entities, e, needs, args.outputDir);
                };
            switch (e.GetEntityKind()) {
                case EntityKind::enum_:
                    addNeedsAsHeaders(std::get<EntityDependentProperties::Enum>(e.dependentProps)
                                          .opaqueEnumDeclarationNeeds);
                    addNeedsAsHeaders(std::get<EntityDependentProperties::Enum>(e.dependentProps)
                                          .declarationNeeds);
                    break;
                case EntityKind::fn:
                    addNeedsAsHeaders(
                        std::get<EntityDependentProperties::Fn>(e.dependentProps).declarationNeeds);
                    break;
                case EntityKind::struct_:
                    addNeedsAsHeaders(
                        std::get<std::to_underlying(EntityKind::struct_)>(e.dependentProps)
                            .forwardDeclarationNeeds);
                    addNeedsAsHeaders(
                        std::get<std::to_underlying(EntityKind::struct_)>(e.dependentProps)
                            .declarationNeeds);
                    break;
                case EntityKind::class_:
                    addNeedsAsHeaders(
                        std::get<std::to_underlying(EntityKind::class_)>(e.dependentProps)
                            .forwardDeclarationNeeds);
                    addNeedsAsHeaders(
                        std::get<std::to_underlying(EntityKind::class_)>(e.dependentProps)
                            .declarationNeeds);
                    break;
                case EntityKind::using_:
                    addNeedsAsHeaders(std::get<EntityDependentProperties::Using>(e.dependentProps)
                                          .declarationNeeds);
                    break;
                case EntityKind::inlvar:
                    addNeedsAsHeaders(std::get<EntityDependentProperties::InlVar>(e.dependentProps)
                                          .declarationNeeds);
                    break;
                case EntityKind::memfn:
                    CHECK(false);  // TODO this is not true, memfn's declaration
                                   // needs go to the class/struct header.
                    addNeedsAsHeaders(std::get<EntityDependentProperties::MemFn>(e.dependentProps)
                                          .declarationNeeds);
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
            case EntityKind::using_:
            case EntityKind::inlvar:
                headerContent += fmt::format("#include \"{}\"\n", e.path);
                break;
            case EntityKind::fn:
                headerContent += fmt::format(
                    "{};\n", std::get<EntityDependentProperties::Fn>(e.dependentProps).declaration);
                break;
            case EntityKind::struct_:
            case EntityKind::class_:
                // TODO (not here) determine source files' relative dires and
                // generate output files according to relative dirs.
                headerContent +=
                    fmt::format("#define {} \"{}\"\n#include \"{}\"\n#undef {}\n",
                                k_nmtIncludeMemberDeclarationsMacro,
                                args.outputDir / path_from_string(e.MemberDeclarationsFilename()),
                                e.path,
                                k_nmtIncludeMemberDeclarationsMacro);
                break;
            case EntityKind::memfn:
                CHECK(false);  // TODO this is not true, memfn declaration goes to
                               // the member function declaration file which gets
                               // injected with a macro.
                headerContent += fmt::format(
                    "{};\n", std::get<EntityDependentProperties::Fn>(e.dependentProps).declaration);
                break;
        }
        fmt::print("Processed {} from {}\n", e.name, e.path);
        auto headerPath = args.outputDir / e.HeaderFilename();
        remainingExistingFiles.erase(headerPath);
        currentFiles.push_back(headerPath);
        auto existingHeaderContent = ReadFile(headerPath);
        if (existingHeaderContent != headerContent) {
            LOG_IF(FATAL, !WriteFile(headerPath, headerContent))
                << fmt::format("Couldn't write {}.", headerPath);
        }  // Else no change, no need to write.

        std::string cppContent =
            fmt::format("{}\n#include \"{}\"\n", k_autogeneratedWarningLine, headerPath);
        switch_variant(
            e.dependentProps,
            [&args, &entities, &cppContent, &e](const EntityDependentProperties::Fn& dp) {
                Includes includes;
                includes.addNeedsAsHeaders(entities, e, dp.definitionNeeds, args.outputDir);
                auto renderedHeaders = includes.render();
                if (!renderedHeaders.empty()) {
                    cppContent += fmt::format("\n{}", renderedHeaders);
                }
            },
            [](auto&&) {});
        cppContent += fmt::format("\n#include \"{}\"\n", e.path);
        auto cppPath = args.outputDir / e.DefinitionFilename();
        remainingExistingFiles.erase(cppPath);
        currentFiles.push_back(cppPath);
        auto existingCppContent = ReadFile(cppPath);
        if (existingCppContent != cppContent) {
            LOG_IF(FATAL, !WriteFile(cppPath, cppContent))
                << fmt::format("Couldn't write {}.", cppPath);
        }  // Else no change, no need to write.
    }      // for a: entities
    for (auto& f : remainingExistingFiles) {
        fs::remove(f, ec);  // Ignore if couldn't remove it.
    }
    std::string fileListContent;
    std::sort(currentFiles.begin(), currentFiles.end());
    for (auto& c : currentFiles) {
        fileListContent += fmt::format("{}\n", c);
    }
    auto filesPath = args.outputDir / "files.txt";
    auto existingFileListContent = ReadFile(filesPath);
    if (existingFileListContent != fileListContent) {
        LOG_IF(FATAL, !WriteFile(filesPath, fileListContent))
            << fmt::format("Could write file list: {}.", filesPath);
    }
    return EXIT_SUCCESS;
}

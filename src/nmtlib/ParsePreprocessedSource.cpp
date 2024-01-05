#include "ParsePreprocessedSource.h"

#include "TokenSearch.h"
#include "nmtutil.h"

namespace fs = std::filesystem;

namespace {
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
                .Eat(TokenType::tok, "::", &doubleColonIdx);
            CHECK(!name.empty());
            if (name[0] == '~') {
                search.Eat(TokenType::tok, "~").Eat(TokenType::id, name.substr(1));
            } else {
                search.Eat(TokenType::id, name);
            }
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
                    "Pattern <class-name>::<memfn-name> not found ({}::{})", *className, name)
                          : fmt::format("Pattern <fn-name> not found (\"{}\")", name));
        }
        if (searchResult.has_value()) {
            // The `(` was also found.
            break;
        }
        // Backtract, start over from where the className was found.
        tokens = tokens.subspan(firstTokenIdx + 1);
    }
    if (openingParenIdx == SIZE_T_MAX) {
        return std::unexpected(fmt::format("Can't find `(`"));
    }
    tokens = tokens.subspan(openingParenIdx);
    size_t openingBraceIdx = SIZE_T_MAX;
    size_t closingParenIdx = SIZE_T_MAX;
    auto searchResult = std::move(TokenSearch(tokens)
                                      .Eat(TokenType::tok, "(")
                                      .Eat(TokenType::tok, ")", &closingParenIdx)
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
    size_t endIdx = SIZE_T_MAX;
    if (className && *className == name) {
        for (auto i = closingParenIdx + 1; i < openingBraceIdx; ++i) {
            if (tokens[i].IsSingleCharToken() && tokens[i].value == ":") {
                endIdx = i;
                break;
            }
        }
    } else {
        endIdx = openingBraceIdx;
    }
    return JoinTokensWithoutComments(std::span<const Token>(tokens0.data(), tokens.data() + endIdx),
                                     exceptTokens)
         + ';';
}

struct Collector {
    std::optional<Visibility> visibility;
    std::optional<EntityKind> entityKind;
    std::vector<std::string> fdneeds, needs, defneeds;
    std::optional<std::string> namespace_;
};

std::expected<Collector, std::vector<std::string>> collectSpecialComments(
    const std::vector<SpecialComment>& specialComments) {
    Collector c;
    std::vector<std::string> errors;
    for (auto& sc : specialComments) {
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
                        errors.push_back(
                            fmt::format("Multiple `#visibility` annotations, previous value: "
                                        "{}, second value: {}",
                                        enum_name<Visibility>(*c.visibility),
                                        fmt::join(sc.list, ", ")));
                        break;
                    }
                    if (sc.list.size() != 1) {
                        errors.push_back(fmt::format("`#visibility` needs a single value, got: {}",
                                                     fmt::join(sc.list, ", ")));
                        break;
                    }
                    c.visibility = enum_from_name<Visibility>(sc.list.front());
                    if (!c.visibility) {
                        errors.push_back(
                            fmt::format("Invalid `#visibility` value: {}", sc.list.front()));
                        break;
                    }
                    break;
                case SpecialCommentKeyword::namespace_:
                    if (c.namespace_) {
                        errors.push_back(
                            fmt::format("Multiple `#namespace` annotations, first value: "
                                        "{}, second value: {}",
                                        *c.namespace_,
                                        fmt::join(sc.list, ", ")));
                        break;
                    }
                    if (sc.list.size() != 1) {
                        errors.push_back(fmt::format("`#namespace` needs a single value, got: {}",
                                                     fmt::join(sc.list, ", ")));
                        break;
                    }
                    c.namespace_ = path_from_string(sc.list.front());
                    break;
            }
        } else if (auto entityKind = enum_from_name<EntityKind>(sc.keyword)) {
            if (!sc.list.empty()) {
                errors.push_back(fmt::format(
                    "Unexpected list after `{}`: {}", sc.keyword, fmt::join(sc.list, ", ")));
                break;
            }
            if (c.entityKind) {
                errors.push_back(fmt::format("Duplicated kind, first: {}, then: {}",
                                             enum_name(*c.entityKind),
                                             enum_name(*entityKind)));
                break;
            }
            c.entityKind = *entityKind;
        } else {
            errors.push_back(fmt::format("Invalid keyword: {}", sc.keyword));
            break;
        }
    }
    if (errors.empty()) {
        return c;
    }
    return std::unexpected(std::move(errors));
}
}  // namespace

std::expected<ParsePreprocessedSourceResult, std::vector<std::string>> ParsePreprocessedSource(
    const PreprocessedSource& pps, const fs::path& sourcePath) {
    std::string name = path_to_string(sourcePath.stem());
    auto containingStructOrClassName = extractContainingStructOrClassNameFromMemberDir(sourcePath);

    TRY_ASSIGN(c, collectSpecialComments(pps.specialComments));
    if (!c.entityKind) {
        return std::unexpected(make_vector(fmt::format(
            "Missing `#<entity>` ({})", fmt::join(enum_traits<EntityKind>::names, ", "))));
    }
    //
    sort_unique_inplace(c.fdneeds);
    sort_unique_inplace(c.needs);
    sort_unique_inplace(c.defneeds);
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
            if (auto cnr = checkNeeds(); !cnr) {
                return std::unexpected(make_vector(std::move(cnr.error())));
            }
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
                return std::unexpected(make_vector(
                    fmt::format("Invalid enum declaration ({})", searchResult.error())));
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
            if (auto cnr = checkNeeds(); !cnr) {
                return std::unexpected(make_vector(std::move(cnr.error())));
            }
            // Expected: ... name ( ... ) ... { ... }
            TRY_ASSIGN_OR_RETURN_VALUE(
                declaration,
                ExtractFunctionDeclaration(std::nullopt, name, tokensFromFirstSpecialComment),
                std::unexpected(make_vector(std::move(UNEXPECTED_ERROR))));
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(EntityKind::fn)>,
                EntityDependentProperties::Fn{.declaration = std::move(declaration),
                                              .declarationNeeds = std::move(c.needs),
                                              .definitionNeeds = std::move(c.defneeds)});
        } break;
        case EntityKind::struct_: {
            allowed.fdneeds = true;
            allowed.needs = true;
            if (auto cnr = checkNeeds(); !cnr) {
                return std::unexpected(make_vector(std::move(cnr.error())));
            }

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
            if (auto cnr = checkNeeds(); !cnr) {
                return std::unexpected(make_vector(std::move(cnr.error())));
            }
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
            if (auto cnr = checkNeeds(); !cnr) {
                return std::unexpected(make_vector(std::move(cnr.error())));
            }
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(EntityKind::header)>,
                EntityDependentProperties::Header{.declarationNeeds = std::move(c.needs)});
        } break;
        case EntityKind::memfn: {
            allowed.needs = true;
            allowed.defneeds = true;
            if (auto cnr = checkNeeds(); !cnr) {
                return std::unexpected(make_vector(std::move(cnr.error())));
            }
            // Expected: ... classname :: name ( ... ) ... { ... }
            TRY_ASSIGN_OR_RETURN_VALUE(
                declaration,
                ExtractFunctionDeclaration(
                    containingStructOrClassName, name, tokensFromFirstSpecialComment),
                std::unexpected(make_vector(std::move(UNEXPECTED_ERROR))));
            dependentProps.emplace(
                std::in_place_index<std::to_underlying(EntityKind::memfn)>,
                EntityDependentProperties::MemFn{.declaration = std::move(declaration),
                                                 .declarationNeeds = std::move(c.needs),
                                                 .definitionNeeds = std::move(c.defneeds)});
        } break;
    }
    CHECK(dependentProps);

    return ParsePreprocessedSourceResult{.name = std::move(name),
                                         .visibility = c.visibility,
                                         .dependentProps = std::move(*dependentProps)};
}

std::expected<DirConfigFile, std::vector<std::string>> ParseDirConfigFile(
    const std::vector<SpecialComment>& specialComments, const std::filesystem::path& sourcePath) {
    CHECK(path_to_string(sourcePath.stem()) == k_dirConfigFileName);

    std::vector<std::string> errors;

    TRY_ASSIGN(c, collectSpecialComments(specialComments));

    if (!sourcePath.has_parent_path()) {
        errors.push_back("Directory config file has no parent path");
    }
    if (errors.empty()) {
        return DirConfigFile{.parentDir = sourcePath.parent_path(),
                             .namespace_ = std::move(c.namespace_)};
    }
    return std::unexpected(std::move(errors));
}

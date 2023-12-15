#include "ParsePreprocessedSource.h"
#include "TokenSearch.h"

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
}  // namespace

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

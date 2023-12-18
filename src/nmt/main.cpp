#include "nmt/GenerateBoilerplate.h"
#include "nmt/ProgramOptions.h"
#include "nmt/Project.h"
#include "nmt/ResolveSourcesFromCommandLine.h"

namespace fs = std::filesystem;

#if 0
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
#endif

int main(int argc, char* argv[]) {
    absl::InitializeLog();

    auto argsOr = ParseProgramOptions(argc, argv);
    if (!argsOr) {
        return argsOr.error();
    }
    auto& args = *argsOr;

    fmt::print("### NMT ###\n");

    auto sourcesOr = ResolveSourcesFromCommandLine(args.sources, args.verbose);
    if (!sourcesOr) {
        fmt::print(stderr, "{}\n", sourcesOr.error());
        return EXIT_FAILURE;
    }
    auto sources = std::move(*sourcesOr);
    for (auto& m : sources.messages) {
        fmt::print("{}\n", m);
    }

    Project project(args.outputDir);
    auto msgsOr = project.AddAndProcessSources(sources.resolvedSources, args.verbose);
    if (!msgsOr) {
        fmt::print(stderr, "{}\n", msgsOr.error());
        return EXIT_FAILURE;
    }
    for (auto& m : *msgsOr) {
        fmt::print("{}\n", m);
    }

    auto gbpr = GenerateBoilerplate(project);
    if (!gbpr) {
        fmt::print(stderr, "{}\n", gbpr.error());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

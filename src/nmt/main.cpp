#include "nmt/GenerateBoilerplate.h"
#include "nmt/ProcessSource.h"
#include "nmt/ProgramOptions.h"
#include "nmt/Project.h"

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

    Project project;
    auto addTargetResult = project.addTarget(args.target, args.sourceDir, args.outputDir);
    if (!addTargetResult) {
        for (auto& m : addTargetResult.error()) {
            fmt::print(stderr, "Error: can't add target {}, reason: ", args.target, m);
        }
        return EXIT_FAILURE;
    }
    std::vector<std::string> verboseMessages, errors;
    for (auto id : project.entities().dirtySources()) {
        auto [newErrors, newVerboseMessages] =
            ProcessSourceAndUpdateProject(project, id, args.verbose);
        append_range(errors, std::move(newErrors));
        if (args.verbose) {
            append_range(verboseMessages, std::move(newVerboseMessages));
        }
    }
    if (args.verbose) {
        for (auto& m : verboseMessages) {
            fmt::print("Note: {}\n", m);
        }
    }
    for (auto& m : errors) {
        fmt::print("ERROR: {}\n", m);
    }
    if (!errors.empty()) {
        return EXIT_FAILURE;
    }

    auto gbpr = GenerateBoilerplate(project);
    if (!gbpr) {
        for (auto& e : gbpr.error()) {
            fmt::print(stderr, "Error: {}\n", e);
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

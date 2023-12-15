// #include "pch.h"

// #include "parse.h"

#include "nmt/GenerateBoilerplate.h"
#include "nmt/Project.h"
// #include "ReadFile.h"
#include "util/ReadFileAsLines.h"
// #include "TokenSearch.h"
// #include "WriteFile.h"
// #include "data.h"
// #include "fn_ExtractIdentifier.h"
// #include "sc_Args.h"

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

struct ProgramOptions {
    bool verbose = false;
    std::vector<std::filesystem::path> sources;
    std::filesystem::path outputDir;
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

    Project project(args.outputDir);

    for (auto& sf : sources) {
        auto r = project.AddAndProcessSource(sf, args.verbose);
        if (!r) {
            fmt::print(stderr, "Failed to process {}: {}\n", sf, r.error());
            return EXIT_FAILURE;
        }
    }

    GenerateBoilerplate(project);

    return EXIT_SUCCESS;
}

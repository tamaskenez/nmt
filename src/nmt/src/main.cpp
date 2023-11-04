#include "pch.h"

#include "PreprocessSource.h"
#include "enum_EntityKind.h"
// #include "fn_EntityKindLongName.h"
// #include "fn_EntityKindOfStem.h"
// #include "fn_EntityKindShortName.h"
// #include "fn_ParseFunctionDeclaration.h"
// #include "fn_ParseOpaqueEnumDeclaration.h"
// #include "fn_ParseStructClassDeclaration.h"
#include "ReadFile.h"
#include "ReadFileAsLines.h"
// #include "fn_WriteFile.h"
// #include "fn_fs_exists_total.h"
// #include "fn_reinterpret_as_u8string_view.h"
#include <absl/container/flat_hash_map.h>
#include "sc_Args.h"
#include "sc_Entity.h"

namespace fs = std::filesystem;

// const char* const k_autogeneratedWarningLine = "// AUTOGENERATED FILE, DO NOT EDIT!";
const std::set<fs::path> k_validSourceExtensions = {".h", ".hpp", ".hxx"};
const std::set<fs::path> k_fileListExtensions = {".txt", ".lst"};

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
            // sources.reserve(sources.size() + pathList->size());
            for (auto& p : *pathList) {
                if (!p.empty()) {
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

std::optional<EntityKind> StringToEntityKind(std::string_view sv) {
    static const std::unordered_map<std::string_view, EntityKind> m = {
        {"enum", EntityKind::enum_},
        {"fn", EntityKind::fn},
        {"struct", EntityKind::struct_},
        {"class", EntityKind::class_},
        {"using", EntityKind::using_},
        {"inlvar", EntityKind::inlvar},
        {"memfn", EntityKind::memfn},
    };

    // Force completeness of map.
    [[maybe_unused]] auto _ = [](EntityKind ek) {
        switch (ek) {
            case EntityKind::enum_:
            case EntityKind::fn:
            case EntityKind::struct_:
            case EntityKind::class_:
            case EntityKind::using_:
            case EntityKind::inlvar:
            case EntityKind::memfn:
                break;
        }
    };

    auto it = m.find(sv);
    if (it == m.end()) {
        return std::nullopt;
    }
    return it->second;
}

enum class NeedsKind { forwardDeclaration, opaqueEnumDeclaration, definition, declaration };

enum class Visibility { public_, target };

struct EntityProperties {
    absl::flat_hash_map<NeedsKind, std::vector<std::string>> needsByKind;
    std::optional<std::string> namespace_;
    Visibility visibility = Visibility::target;
};

void ParsePreprocessedSource(const PreprocessedSource& pps) {
    for (auto& sc : pps.specialComments) {
                if(sc.keyword=="
    }
}

int main(int argc, char* argv[]) {
    absl::InitializeLog();

    CLI::App app("C++ boilerplate generator", "NMT");

    Args args;

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
    app.add_option("-v,--verbose", args.verbose, "Print more diagnostics");

    CLI11_PARSE(app, argc, argv);

    auto sourcesOr = ResolveArgsSources(args.sources, args.verbose);
    if (!sourcesOr) {
                return EXIT_FAILURE;
    }
    auto sources = std::move(*sourcesOr);

    fmt::print("sources: {}\n", fmt::join(sources, ", "));

    int result = EXIT_SUCCESS;

    std::unordered_map<std::string, Entity> entities;

    for (auto& sf : args.sources) {
                auto maybeSource = ReadFile(sf);
                if (!maybeSource) {
                    fmt::print(stderr, "Can't open file for reading: `{}`", sf);
                    return EXIT_FAILURE;
                }
                auto& source = *maybeSource;

                auto ppsOr = PreprocessSource(source);
                if (!ppsOr) {
                    fmt::print(stderr, "{} in {}", ppsOr.error(), sf);
                    return EXIT_FAILURE;
                }

                auto pppsOr = ParsePreprocessedSource(*ppsOr);

                for (auto ncc : ppsOr->nonCommentCode) {
                    fmt::print("--- BEGIN NCC--- \n{}\n--- END NCC ---\n\n", ncc);
                }
                for (auto& sc : ppsOr->specialComments) {
                    fmt::print("--- BEGIN SC `{}` ---\n", sc.keyword);
                    for (auto& x : sc.list) {
                        fmt::print("\t `{}`\n", x);
                    }
                    fmt::print("--- END SC `{}` ---\n", sc.keyword);
                }
                fmt::print("\n");

                [[maybe_unused]] auto& pps = *ppsOr;
                fmt::print("\n");

#if 0

        auto kind = EntityKindOfStem(sf.stem().string());
        LOG_IF(FATAL, !kind) << fmt::format(
            "Source file name `{}` is not prefixed by Entitykind (in file `{}`)",
            sf.filename(),
            sf);

        std::vector<std::string> declNeeds, defNeeds;
        for (auto& sc : ppsOr->specialComments) {
            auto& v = sc.posInPPSource <= 0 ? declNeeds : defNeeds;
            v.insert(v.end(),
                     std::make_move_iterator(sc.needs.begin()),
                     std::make_move_iterator(sc.needs.end()));
        }

        auto f = [&declNeeds, &defNeeds, &sf, &entities](
                     std::expected<std::pair<std::string, std::string>, std::string> edOr,
                     EntityKind kind) {
            LOG_IF(FATAL, !edOr) << fmt::format("Can't parse {}: int file `{}`, reason: {}",
                                                EntityKindLongName(kind),
                                                sf,
                                                edOr.error());
            auto entity = Entity{.kind = kind,
                                 .name = edOr->first,
                                 .lightDeclaration = std::move(edOr->second),
                                 .path = sf,
                                 .declNeeds = std::move(declNeeds),
                                 .defNeeds = std::move(defNeeds)};
            auto itb = entities.insert(std::make_pair(std::move(edOr->first), std::move(entity)));
            LOG_IF(FATAL, !itb.second) << fmt::format("Duplicate name: `{}`, first {} then {}.",
                                                      entity.name,
                                                      EntityKindLongName(itb.first->second.kind),
                                                      EntityKindLongName(kind));
        };

        switch (*kind) {
            case EntityKind::enum_:
                f(ParseOpaqueEnumDeclaration(ppsOr->ppSource), EntityKind::enum_);
                break;
            case EntityKind::fn:
                f(ParseFunctionDeclaration(ppsOr->ppSource), EntityKind::fn);
                break;
            case EntityKind::sc:
                f(ParseStructClassDeclaration(ppsOr->ppSource), EntityKind::sc);
                break;
        }
    }

    std::error_code ec;
    fs::create_directories(args.outputDir, ec);
    LOG_IF(FATAL, ec) << fmt::format("Can't create output directory `{}`.", args.outputDir);

    auto dit = fs::directory_iterator(args.outputDir, fs::directory_options::none, ec);
    LOG_IF(FATAL, ec) << fmt::format("Can't get listing of output directory `{}`.", args.outputDir);
    struct path_hash {
        size_t operator()(const fs::path p) const {
            return hash_value(p);
        }
    };
    std::unordered_set<fs::path, path_hash> remainingExistingFiles;
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
        void addNeedsAsHeaders(const std::unordered_map<std::string, Entity>& entities,
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
            const std::unordered_map<std::string, Entity>& entities,
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
                    auto it = entities.find(
                        std::string(needName));  // TODO(optimize): could use heterogenous lookup.
                    LOG_IF(FATAL, it == entities.end()) << fmt::format(
                        "Entity `{}` needs `{}` but it's missing.", e.name, needName);
                    auto& ne = it->second;
                    switch (ne.kind) {
                        case EntityKind::enum_:
                        case EntityKind::sc:
                            break;
                        case EntityKind::fn:
                            LOG_IF(FATAL, refOnly) << fmt::format(
                                "Entity `{}` needs `{}` but it's a function and can't "
                                "be forward declared.",
                                e.name,
                                need);
                            break;
                    }
                    if (refOnly) {
                        forwardDeclarations.push_back(fmt::format("{};", ne.lightDeclaration));
                        additionalNeeds.insert(
                            additionalNeeds.end(), ne.declNeeds.begin(), ne.declNeeds.end());
                    } else {
                        auto filename =
                            fmt::format("{}_{}.h", EntityKindShortName(ne.kind), ne.name);
                        auto path = outputDir / fs::path(reinterpret_as_u8string_view(filename));
                        generateds.push_back(fmt::format("\"{}\"", path));
                    }
                }
            }
            return additionalNeeds;
        }
#endif
    }
#if 0
    for (auto& [_, e] : entities) {
        std::string headerContent = fmt::format("{}\n#pragma once\n", k_autogeneratedWarningLine);
        {
            Includes includes;
            includes.addNeedsAsHeaders(entities, e, e.declNeeds, args.outputDir);
            switch (e.kind) {
                case EntityKind::enum_:
                    includes.addNeedsAsHeaders(entities, e, e.defNeeds, args.outputDir);
                case EntityKind::fn:
                case EntityKind::sc:
                    break;
            }
            auto renderedHeaders = includes.render();
            if (!renderedHeaders.empty()) {
                headerContent += fmt::format("\n{}", renderedHeaders);
            }
        }
        headerContent += "\n";
        switch (e.kind) {
            case EntityKind::enum_:
            case EntityKind::sc:
                headerContent += fmt::format("#include \"{}\"\n", e.path);
                break;
            case EntityKind::fn:
                headerContent += fmt::format("{};\n", e.lightDeclaration);
                break;
        }
        fmt::print("Processed {} from {}\n", e.name, e.path);
        auto headerPath =
            args.outputDir / fmt::format("{}_{}.h", EntityKindShortName(e.kind), e.name);
        remainingExistingFiles.erase(headerPath);
        currentFiles.push_back(headerPath);
        auto existingHeaderContent = ReadFile(headerPath);
        if (existingHeaderContent != headerContent) {
            LOG_IF(FATAL, !WriteFile(headerPath, headerContent))
                << fmt::format("Couldn't write {}.", headerPath);
        }  // Else no change, no need to write.

        switch (e.kind) {
            case EntityKind::enum_:
                // defNeeds were processed into the header.
                break;
            case EntityKind::sc:
                LOG_IF(FATAL, !e.defNeeds.empty())
                    << fmt::format("Struct/class `{}` is not supposed to have defNeeds.\n", e.name);
                break;
            case EntityKind::fn: {
                std::string cppContent =
                    fmt::format("{}\n#include \"{}\"\n", k_autogeneratedWarningLine, headerPath);
                {
                    Includes includes;
                    includes.addNeedsAsHeaders(entities, e, e.defNeeds, args.outputDir);
                    auto renderedHeaders = includes.render();
                    if (!renderedHeaders.empty()) {
                        cppContent += fmt::format("\n{}", renderedHeaders);
                    }
                }
                cppContent += fmt::format("\n#include \"{}\"\n", e.path);
                auto cppPath =
                    args.outputDir / fmt::format("{}_{}.cpp", EntityKindShortName(e.kind), e.name);
                remainingExistingFiles.erase(cppPath);
                currentFiles.push_back(cppPath);
                auto existingCppContent = ReadFile(cppPath);
                if (existingCppContent != cppContent) {
                    LOG_IF(FATAL, !WriteFile(cppPath, cppContent))
                        << fmt::format("Couldn't write {}.", cppPath);
                }  // Else no change, no need to write.
            } break;
        }
    }
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

#endif
    return result;
}
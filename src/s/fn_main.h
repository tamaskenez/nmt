int main(int argc, char* argv[])
// needs: <cstdlib>, <glog/logging.h>, <fmt/std.h>, ReadArgs, EntityKindOfStem, ReadFileAsLines
// needs: PreprocessSource, EntityKind, ParseOpaqueEnumDeclaration
// needs: EntityKindLongName, EntityKindShortName
// needs: ParseFunctionDeclaration
// needs: ParseStructClassDeclaration
// needs: ReadFileAsLines, Entity, <iterator>
// needs: reinterpret_as_u8string_view, <unordered_set>, WriteFile
{
    namespace fs = std::filesystem;

    google::InitGoogleLogging(argv[0]);
    // Read args.
    auto args = ReadArgs(argc, argv);
    fmt::print("-s: {}\n", fmt::join(args.sources, ", "));
    fmt::print("-f: {}\n", fmt::join(args.sourcesFiles, ", "));
    fmt::print("-o: {}\n", args.outputDir);

    for (auto& f : args.sourcesFiles) {
        auto pathList = ReadFileAsLines(f);
        LOG_IF(FATAL, !pathList) << fmt::format("Can't read file list from {}.", f);
        args.sources.insert(args.sources.end(),
                            std::make_move_iterator(pathList->begin()),
                            std::make_move_iterator(pathList->end()));
    }

    int result = EXIT_SUCCESS;

    std::unordered_map<std::string, Entity> entities;

    // Canonicalize source paths.
    for (auto& sf : args.sources) {
        std::error_code ec;
        auto c = fs::canonical(sf, ec);
        LOG_IF(FATAL, ec) << fmt::format("Source not found ({}): {}", ec.message(), sf);
        sf = c;
    }

    for (auto& sf : args.sources) {
        // Find kind.
        auto maybeSource = ReadFile(sf);
        LOG_IF(FATAL, !maybeSource) << fmt::format("Can't open file for reading: {}", sf);
        auto& source = *maybeSource;

        auto ppsOr = PreprocessSource(source);
        if (!ppsOr) {
            LOG(FATAL) << fmt::format("{} in {}", ppsOr.error(), sf);
        }

        auto kind = EntityKindOfStem(sf.stem().string());
        LOG_IF(FATAL, !kind) << fmt::format(
            "Source file name {} is not prefixed by Entitykind ({})", sf.filename(), sf);

        std::vector<std::string> declNeeds, defNeeds;
        for (auto& sc : ppsOr->specialComments) {
            auto& v = sc.posInPPSource <= 0 ? declNeeds : defNeeds;
            v.insert(v.end(),
                     std::make_move_iterator(sc.needs.begin()),
                     std::make_move_iterator(sc.needs.end()));
        }

        auto f = [&declNeeds, &defNeeds, &sf, &entities](
                     nonstd::expected<std::pair<std::string, std::string>, std::string> edOr,
                     EntityKind kind) {
            LOG_IF(FATAL, !edOr) << fmt::format("Can't parse {}: {}", EntityKindLongName(kind), sf);
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
    LOG_IF(FATAL, ec) << fmt::format("Can't create output directory {}.", args.outputDir);

    auto dit = fs::directory_iterator(args.outputDir, fs::directory_options::none, ec);
    LOG_IF(FATAL, ec) << fmt::format("Can't get listing of output directory {}.", args.outputDir);
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

    for (auto& [_, e] : entities) {
        std::vector<std::string> generateds, locals, externalsInDirs, externalWithExtension,
            externalsWithoutExtension, forwardDeclarations;
        auto addHeader = [&locals,
                          &externalsInDirs,
                          &externalWithExtension,
                          &externalsWithoutExtension](std::string_view s) {
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
        std::vector<std::string> declNeedsOfOpaqueEnumDeclarations;
        for (auto pass : {0, 1}) {
            const auto& needs = pass == 0 ? e.declNeeds : declNeedsOfOpaqueEnumDeclarations;
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
                        << fmt::format("Entity {} can't include itself.", e.name);
                    auto it = entities.find(
                        std::string(needName));  // TODO(optimize): could use heterogenous lookup.
                    LOG_IF(FATAL, it == entities.end())
                        << fmt::format("Entity {} needs {} but it's missing.", e.name, needName);
                    auto& ne = it->second;
                    switch (ne.kind) {
                        case EntityKind::enum_:
                        case EntityKind::sc:
                            break;
                        case EntityKind::fn:
                            LOG_IF(FATAL, refOnly)
                                << fmt::format("Entity {} needs {} but it's a function and can't "
                                               "be forward declared.",
                                               e.name,
                                               need);
                            break;
                    }
                    if (refOnly) {
                        forwardDeclarations.push_back(fmt::format("{};", ne.lightDeclaration));
                        declNeedsOfOpaqueEnumDeclarations.insert(
                            declNeedsOfOpaqueEnumDeclarations.end(),
                            ne.declNeeds.begin(),
                            ne.declNeeds.end());
                    } else {
                        auto filename =
                            fmt::format("{}_{}.h", EntityKindShortName(ne.kind), ne.name);
                        auto path =
                            args.outputDir / fs::path(reinterpret_as_u8string_view(filename));
                        generateds.push_back(fmt::format("\"{}\"", path));
                    }
                }
            }
        }
        std::string content = fmt::format("#pragma once\n");
        auto addHeaders = [&content](std::vector<std::string>& v) {
            if (!v.empty()) {
                content += "\n";
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
            content += "\n";
            std::sort(forwardDeclarations.begin(), forwardDeclarations.end());
            for (auto& s : forwardDeclarations) {
                content += s;
            }
        }
        content += "\n";
        switch (e.kind) {
            case EntityKind::enum_:
            case EntityKind::sc:
                content += fmt::format("#include \"{}\"\n", e.path);
                break;
            case EntityKind::fn:
                content += fmt::format("{};\n", e.lightDeclaration);
                break;
        }
        fmt::print("---- name: {} ----\n{}\n", e.name, content);
        auto headerPath =
            args.outputDir / fmt::format("{}_{}.h", EntityKindShortName(e.kind), e.name);
        remainingExistingFiles.erase(headerPath);
        currentFiles.push_back(headerPath);
        auto existingHeaderContent = ReadFile(headerPath);
        if (existingHeaderContent == content) {
            // No change, no need to write.
            continue;
        }
        LOG_IF(FATAL, !WriteFile(headerPath, content))
            << fmt::format("Couldn't write {}.", headerPath);
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

    return result;
}

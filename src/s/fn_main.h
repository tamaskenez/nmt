int main(int argc, char* argv[])
// needs: <cstdlib>, <glog/logging.h>, <fmt/std.h>, ReadArgs, EntityKindOfStem, ReadFileAsLines,
// needs: PreprocessSource, EntityKind, ParseOpaqueEnumDeclaration
// needs: EntityKindLongName
// needs: ParseFunctionDeclaration
// needs: ParseStructClassDeclaration
{
    namespace fs = std::filesystem;

    google::InitGoogleLogging(argv[0]);
    // Read args.
    auto args = ReadArgs(argc, argv);
    fmt::print("-s: {}\n", fmt::join(args.sourceFiles, ", "));
    fmt::print("-o: {}\n", args.outputDir);
    int result = EXIT_SUCCESS;

    struct Entity {
        EntityKind kind;
        std::string name;
        // lightDeclaration is
        // - opaque-enum-declaration for enums
        // - forward-declaration for structs, classes
        // - function-declaration for functions
        std::string lightDeclaration;
        std::filesystem::path path;
    };
    std::unordered_map<std::string, Entity> entities;

    for (auto& sf : args.sourceFiles) {
        // Find kind.
        auto maybeSource = ReadFile(sf);
        LOG_IF(FATAL, !maybeSource) << fmt::format("Can't open file for reading: {}", sf);
        auto& source = *maybeSource;

        auto ppsOr = PreprocessSource(source);
        if (!ppsOr) {
            LOG(FATAL) << fmt::format("{} in {}", ppsOr.error(), sf);
        }
        // auto& pps = *ppsOr;

        auto kind = EntityKindOfStem(sf.stem().string());
        LOG_IF(FATAL, !kind) << fmt::format(
            "Source file name {} is not prefixed by Entitykind ({})", sf.filename(), sf);
        switch (*kind) {
            case EntityKind::enum_: {
                auto edOr = ParseOpaqueEnumDeclaration(ppsOr->ppSource);
                LOG_IF(FATAL, !edOr) << fmt::format("Can't parse enum: {}", sf);
                auto entity = Entity{.kind = EntityKind::enum_,
                                     .name = edOr->first,
                                     .lightDeclaration = std::move(edOr->second)};
                auto itb =
                    entities.insert(std::make_pair(std::move(edOr->first), std::move(entity)));
                LOG_IF(FATAL, !itb.second)
                    << fmt::format("Duplicate name: `{}`, first {} then enum.",
                                   entity.name,
                                   EntityKindLongName(itb.first->second.kind));
            } break;
            case EntityKind::fn: {
                auto edOr = ParseFunctionDeclaration(ppsOr->ppSource);
                LOG_IF(FATAL, !edOr) << fmt::format("Can't parse function: {}", sf);
                auto entity = Entity{.kind = EntityKind::fn,
                                     .name = edOr->first,
                                     .lightDeclaration = std::move(edOr->second)};
                auto itb =
                    entities.insert(std::make_pair(std::move(edOr->first), std::move(entity)));
                LOG_IF(FATAL, !itb.second)
                    << fmt::format("Duplicate name: `{}`, first {} then function.",
                                   entity.name,
                                   EntityKindLongName(itb.first->second.kind));
            } break;
            case EntityKind::sc: {
                auto edOr = ParseStructClassDeclaration(ppsOr->ppSource);
                LOG_IF(FATAL, !edOr) << fmt::format("Can't parse struct/class: {}", sf);
                auto entity = Entity{.kind = EntityKind::sc,
                                     .name = edOr->first,
                                     .lightDeclaration = std::move(edOr->second)};
                auto itb =
                    entities.insert(std::make_pair(std::move(edOr->first), std::move(entity)));
                LOG_IF(FATAL, !itb.second)
                    << fmt::format("Duplicate name: `{}`, first {} then struct/class.",
                                   entity.name,
                                   EntityKindLongName(itb.first->second.kind));
            } break;
        }
    }
    return result;
}

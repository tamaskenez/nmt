#include "nmt/GenerateBoilerplate.h"

#include "GeneratedFileWriter.h"
#include "nmt/Project.h"

namespace fs = std::filesystem;

namespace {

std::optional<std::string_view> ExtractIdentifier(std::string_view sv) {
    auto r = trim(sv);
    if (r.empty()) {
        return std::nullopt;
    }
    if (isdigit(r.front())) {
        return std::nullopt;
    }
    for (auto c : r) {
        if (!isalnum(c) && c != '_') {
            return std::nullopt;
        }
    }
    return r;
}

struct IncludeSectionBuilder {
    explicit IncludeSectionBuilder(const Project& project)
        : project(project) {}
    void addNeedsAsHeaders(const Entities& entities,
                           const Entity& e,
                           const std::vector<std::string>& needs) {
        DCHECK(!failedAndErrorsHasBeenReturned);
        std::vector<std::string> additionalNeeds;
        const std::vector<std::string>* currentNeeds = &needs;
        while (!currentNeeds->empty()) {
            additionalNeeds = addNeedsAsHeadersCore(entities, e, *currentNeeds);
            // TODO: prevent infinite loop because of some circular dependency.
            currentNeeds = &additionalNeeds;
        }
    }
    std::expected<std::string, std::vector<std::string>> render() {
        CHECK(!failedAndErrorsHasBeenReturned);
        if (!errors.empty()) {
            failedAndErrorsHasBeenReturned = true;
            return std::unexpected(std::move(errors));
        }
        std::string content;
        auto addHeaders = [&content](std::vector<std::string>& v) {
            if (!v.empty()) {
                if (!content.empty()) {
                    content += "\n";
                }
                sort_unique_inplace(v);
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
            sort_unique_inplace(forwardDeclarations);
            for (auto& s : forwardDeclarations) {
                content += fmt::format("{}\n", s);
            }
        }
        return content;
    }

   private:
    const Project& project;
    bool failedAndErrorsHasBeenReturned = false;
    std::vector<std::string> generateds, locals, externalsInDirs, externalWithExtension,
        externalsWithoutExtension, forwardDeclarations;
    std::vector<std::string> errors;
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
    // Return additional needs coming from the enums which have needs for their
    // opaque-enum-declaration.
    std::vector<std::string> addNeedsAsHeadersCore(const Entities& entities,
                                                   const Entity& e,
                                                   const std::vector<std::string>& needs) {
        std::vector<std::string> additionalNeeds;
        for (auto& need : needs) {
            LOG_IF(FATAL, need.empty()) << "Empty need name.";
            if (need[0] == '<' || need[0] == '"') {
                addHeader(need);
            } else {
                if (need.starts_with("struct ") || need.starts_with("class ")
                    || need.starts_with("enum ")) {
                    auto spaceIdx = need.find(' ');
                    assert(spaceIdx != std::string_view::npos);
                    auto identifier =
                        ExtractIdentifier(need.substr(spaceIdx, need.size() - spaceIdx));
                    if (!identifier) {
                        errors.push_back(
                            fmt::format("Forward declaration {} needs a c-identifier", need));
                        continue;
                    }
                    forwardDeclarations.push_back(
                        fmt::format("{} {};", need.substr(0, spaceIdx), *identifier));
                } else {
                    std::string_view needName = need;
                    const bool refOnly = needName.back() == '*';
                    if (refOnly) {
                        needName.remove_suffix(1);
                    }
                    if (needName == e.name) {
                        errors.push_back(fmt::format("Entity `{}` can't include itself.", e.name));
                        continue;
                    }
                    auto maybeId = entities.findNonMemberByName(e.targetId, needName);
                    if (!maybeId) {
                        errors.push_back(fmt::format(
                            "Entity `{}` needs `{}` but it's missing.", e.name, needName));
                        continue;
                    }
                    auto& ne = entities.entity(*maybeId);
                    if (refOnly) {
                        auto* v = ne.ForwardDeclarationNeedsOrNull();
                        if (v == nullptr) {
                            errors.push_back(
                                fmt::format("Entity `{}` needs `{}` but it's a {} and can't "
                                            "be forward declared.",
                                            e.name,
                                            need,
                                            enum_name(ne.GetEntityKind())));
                            continue;
                        }
                        additionalNeeds.insert(additionalNeeds.end(), BEGIN_END(*v));
                        forwardDeclarations.push_back(fmt::format("{}", ne.ForwardDeclaration()));
                    } else {
                        generateds.push_back(
                            fmt::format("\"{}\"", project.headerPath(false, *maybeId)));
                    }
                }
            }
        }
        return additionalNeeds;
    }
};
/*
struct TargetSubdirOfSourcePath {
    std::vector<std::pair<fs::path, fs::path>> parentDirToSubdir;
    template<class PathToDirConfigFileMap>
    explicit TargetSubdirOfSourcePath(const PathToDirConfigFileMap& dirConfigFiles) {
        parentDirToSubdir.reserve(dirConfigFiles.size());
        for (auto& [k, v] : dirConfigFiles) {
            if (v.subdir) {
                parentDirToSubdir.push_back(std::make_pair(v.parentDir, *v.subdir));
            }
        }
        std::ranges::sort(parentDirToSubdir);
        for (size_t i = 0; i < parentDirToSubdir.size(); ++i) {
            DCHECK(parentDirToSubdir[i - 1].first != parentDirToSubdir[i].first) << fmt::format(
                "Duplicated parent paths for dir config files: {}", parentDirToSubdir[i - 1].first);
        }
    }
    const fs::path& subdir(const fs::path& sourcePath) const {
        static const fs::path k_emptyPath;
        if (parentDirToSubdir.empty() || !sourcePath.has_parent_path()) {
            return k_emptyPath;
        }
        auto parentPath = sourcePath.parent_path();
        auto it = std::ranges::upper_bound(
            parentDirToSubdir, parentPath, {}, [](auto&& x) -> const fs::path& {
                return x.first;
            });
        if (it == std::ranges::begin(parentDirToSubdir)) {
            return k_emptyPath;
        }
        --it;  // There is always a previous item because parentDirToSubdir is not empty.
        auto mr = std::ranges::mismatch(it->first, parentPath);
        bool dirConfigFileParentIsPrefixOfSourcePath = mr.in1 == it->first.end();
        return dirConfigFileParentIsPrefixOfSourcePath ? it->second : k_emptyPath;
    }
};
*/
}  // namespace

std::expected<std::monostate, std::vector<std::string>> GenerateBoilerplate(
    const Project& project) {
    node_hash_map<fs::path, GeneratedFileWriter, path_hash> gfws;
    std::vector<std::string> errors;

    auto addGfw = [&](const fs::path& outputDir, int64_t targetId) -> GeneratedFileWriter& {
        auto it = gfws.find(outputDir);
        if (it == gfws.end()) {
            auto& gfw = gfws.insert(std::make_pair(outputDir, GeneratedFileWriter(outputDir)))
                            .first->second;
            // Generate empty header.
            gfw.Write(project.emptyHeaderPath(true, targetId),
                      fmt::format(
                          "{}\n{}\n", k_autogeneratedWarningLine, k_IntentionallyEmptyLineWarning));
            return gfw;
        }
        return it->second;
    };

    auto entityIds = project.entities().itemsWithEntities();

    /*
    TargetSubdirOfSourcePath targetSubdirOfSourcePath(project.dirConfigFiles);
    auto targetSubdirOfSourcePathFn = [&targetSubdirOfSourcePath](const fs::path& x) {
        return targetSubdirOfSourcePath.subdir(x);
    };
*/

    flat_hash_map<int64_t, std::vector<Entities::Id>> containingEntityToMembersMap;
    flat_hash_map<int64_t, int64_t> membersToContainingEntityMap;
    {
        for (auto id : entityIds) {
            auto& e = project.entities().entity(id);
            if (auto* memfn = std::get_if<EntityDependentProperties::MemFn>(&e.dependentProps)) {
                if (e.sourcePath.has_parent_path()) {
                    auto parentPath = e.sourcePath.parent_path();
                    std::vector<int64_t> matchingContainingEntities;
                    std::vector<fs::path> matchingContainingSourceFiles;
                    for (auto hx : k_validSourceExtensions) {
                        auto testPath = parentPath;
                        testPath.replace_extension(hx);
                        if (auto maybeId =
                                project.entities().findEntityBySourcePath(e.targetId, testPath)) {
                            matchingContainingEntities.push_back(*maybeId);
                            matchingContainingSourceFiles.push_back(std::move(testPath));
                        }
                    }
                    switch (matchingContainingEntities.size()) {
                        case 0:
                            errors.push_back(
                                fmt::format("Member function source file `{}`: no containing "
                                            "class/struct source file found, extensions tried: {}",
                                            e.sourcePath,
                                            fmt::join(k_validSourceExtensions, ", ")));
                            break;
                        case 1: {
                            containingEntityToMembersMap[matchingContainingEntities.front()]
                                .push_back(id);
                            auto itb = membersToContainingEntityMap.insert(
                                std::make_pair(id, matchingContainingEntities.front()));
                            CHECK(itb.second)
                                << fmt::format("Entity #{} has 2 containing entities: #{} and #{}",
                                               id,
                                               itb.first->second,
                                               matchingContainingEntities.front());
                        } break;
                        default:
                            errors.push_back(
                                fmt::format("Member function source file `{}`: multiple containing "
                                            "class/struct source files found: {}",
                                            e.sourcePath,
                                            fmt::join(matchingContainingSourceFiles, ", ")));
                            break;
                    }
                } else {
                    errors.push_back(fmt::format(
                        "Member function source file `{}` has no parent path", e.sourcePath));
                }
            }
        }
    }
    for (auto id : entityIds) {
        auto& e = project.entities().entity(id);
        auto targetIt = project.targets().find(e.targetId);
        CHECK(targetIt != project.targets().end());
        auto& target = targetIt->second;
        auto& gfw = addGfw(target.outputDir, e.targetId);
        bool generateHeader;
        switch (e.GetEntityKind()) {
            case EntityKind::enum_:
            case EntityKind::fn:
            case EntityKind::struct_:
            case EntityKind::class_:
            case EntityKind::header:
                generateHeader = true;
                break;
            case EntityKind::memfn:
                generateHeader = false;
                break;
        }
        if (generateHeader) {
            std::string headerContent =
                fmt::format("{}\n#pragma once\n", k_autogeneratedWarningLine);
            {
                IncludeSectionBuilder includes(project);
                auto addNeedsAsHeaders =
                    [&includes, &e, &project](const std::vector<std::string>& needs) {
                        includes.addNeedsAsHeaders(project.entities(), e, needs);
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
                        auto it = containingEntityToMembersMap.find(id);
                        if (it != containingEntityToMembersMap.end()) {
                            for (auto& mfid : it->second) {
                                auto& me = project.entities().entity(mfid);
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
                        auto it = containingEntityToMembersMap.find(id);
                        if (it != containingEntityToMembersMap.end()) {
                            for (auto& mfid : it->second) {
                                auto& me = project.entities().entity(mfid);
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
                auto renderedHeadersOr = includes.render();
                if (!renderedHeadersOr) {
                    for (auto& error : renderedHeadersOr.error()) {
                        errors.push_back(
                            fmt::format("Failed generating boilerplate for {}, reason: {}",
                                        e.sourcePath,
                                        error));
                    }
                    continue;
                }
                auto& renderedHeaders = *renderedHeadersOr;
                if (!renderedHeaders.empty()) {
                    headerContent += fmt::format("\n{}", renderedHeaders);
                }
            }
            headerContent += "\n";
            switch (e.GetEntityKind()) {
                case EntityKind::enum_:
                case EntityKind::header:
                    headerContent += fmt::format("#include \"{}\"\n", e.sourcePath);
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
                    auto it = containingEntityToMembersMap.find(id);
                    fs::path memberDeclarationsPath;
                    if (it == containingEntityToMembersMap.end()) {
                        memberDeclarationsPath = project.emptyHeaderPath(true, e.targetId);
                    } else {
                        std::string mdContent = fmt::format("{}\n\n", k_autogeneratedWarningLine);
                        for (auto& m : k_memberDefinitionMacros) {
                            mdContent += fmt::format("#define {} {}\n", m.name, m.forHeader);
                        }
                        mdContent += "\n";
                        for (auto& mfid : it->second) {
                            auto& mfe = project.entities.entity(mfid);
                            auto& dp =
                                std::get<std::to_underlying(EntityKind::memfn)>(mfe.dependentProps);
                            mdContent += fmt::format("{}\n", dp.declaration);
                        }
                        mdContent += "\n";
                        for (auto& m : k_memberDefinitionMacros) {
                            mdContent += fmt::format("#undef {}\n", m.name);
                        }
                        memberDeclarationsPath = project.memberDeclarationsPath(true, id);
                        gfw.Write(memberDeclarationsPath, mdContent);
                    }
                    headerContent += fmt::format("#define {} \"{}\"\n#include \"{}\"\n#undef {}\n",
                                                 k_nmtIncludeMemberDeclarationsMacro,
                                                 target.outputDir / memberDeclarationsPath,
                                                 e.sourcePath,
                                                 k_nmtIncludeMemberDeclarationsMacro);
                } break;
                case EntityKind::memfn:
                    // memfn declaration goes to the member function declaration file which gets
                    // injected with a macro.
                    CHECK(false);
                    break;
            }

            fmt::print("Processed {} from {}\n", e.name, e.sourcePath);
            gfw.Write(project.headerPath(true, id), headerContent);
        }
        std::string cppContent;
        bool cppContentProductionFailed = false;
        switch_variant(
            e.dependentProps,
            [&](const EntityDependentProperties::Fn& dp) {
                cppContent += fmt::format("{}\n#include \"{}\"\n",
                                          k_autogeneratedWarningLine,
                                          project.headerPath(false, id));
                IncludeSectionBuilder includes(project);

                includes.addNeedsAsHeaders(project.entities, e, dp.definitionNeeds);
                auto renderedHeadersOr = includes.render();
                if (!renderedHeadersOr) {
                    append_range(errors, std::move(renderedHeadersOr.error()));
                    return;
                }
                auto& renderedHeaders = *renderedHeadersOr;
                if (!renderedHeaders.empty()) {
                    cppContent += fmt::format("\n{}", renderedHeaders);
                }
                cppContent += fmt::format("\n#include \"{}\"\n", e.sourcePath);
            },
            [&](const EntityDependentProperties::MemFn& dp) {
                auto ceIt = membersToContainingEntityMap.find(id);
                CHECK(ceIt != membersToContainingEntityMap.end())
                    << fmt::format("Containing struct/class not found for `{}`", e.sourcePath);
                auto ceId = ceIt->second;
                cppContent += fmt::format("{}\n#include \"{}\"\n",
                                          k_autogeneratedWarningLine,
                                          project.headerPath(false, ceId));
                IncludeSectionBuilder includes(project);
                includes.addNeedsAsHeaders(project.entities, e, dp.definitionNeeds);
                auto renderedHeadersOr = includes.render();
                if (!renderedHeadersOr) {
                    append_range(errors, std::move(renderedHeadersOr.error()));
                    return;
                }
                auto& renderedHeaders = *renderedHeadersOr;
                if (!renderedHeaders.empty()) {
                    cppContent += fmt::format("\n{}", renderedHeaders);
                }
                cppContent += "\n";
                for (auto& m : k_memberDefinitionMacros) {
                    cppContent += fmt::format("#define {} {}\n", m.name, m.forCpp);
                }
                cppContent += fmt::format("\n#include \"{}\"\n\n", e.sourcePath);
                for (auto& m : k_memberDefinitionMacros) {
                    cppContent += fmt::format("#undef {}\n", m.name);
                }
            },
            [&](const auto&) {
                cppContent += fmt::format("{}\n#include \"{}\"\n",
                                          k_autogeneratedWarningLine,
                                          project.headerPath(false, id));
            });
        if (cppContentProductionFailed) {
            continue;
        }
        gfw.Write(project.cppPath(true, id), cppContent);
    }  // for a: entities
    flat_hash_set<fs::path, path_hash> generatedFiles;
    for (auto& [k, gfw] : gfws) {
        std::ranges::sort(gfw.currentFiles);
        for (auto& cf : gfw.currentFiles) {
            auto itb = generatedFiles.insert(cf);
            if (!itb.second) {
                errors.push_back(fmt::format(
                    "The generated file `{}` was written for 2 separate source files.", cf));
            }
        }
        std::string fileListContent;
        for (auto& c : gfw.currentFiles) {
            fileListContent += fmt::format("{}\n", c);
        }
        gfw.Write(k_fileListFilename, fileListContent);
        gfw.RemoveRemainingExistingFilesAndDirs();
    }
    if (errors.empty()) {
        return {};
    } else {
        return std::unexpected(std::move(errors));
    }
}

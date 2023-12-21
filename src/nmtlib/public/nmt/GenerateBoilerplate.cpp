#include "nmt/GenerateBoilerplate.h"

#include "GeneratedFileWriter.h"
#include "nmt/Project.h"

namespace fs = std::filesystem;

namespace {
struct IncludeSectionBuilder {
    IncludeSectionBuilder(
        fs::path outputDir,
        std::function<const fs::path&(const fs::path&)> sourcePathToTargetSubdirFn)
        : outputDir(std::move(outputDir))
        , sourcePathToTargetSubdirFn(std::move(sourcePathToTargetSubdirFn)) {}
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
    fs::path outputDir;
    std::function<const fs::path&(const fs::path&)> sourcePathToTargetSubdirFn;
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
                std::string_view needName = need;
                const bool refOnly = needName.back() == '*';
                if (refOnly) {
                    needName.remove_suffix(1);
                }
                if (needName == e.name) {
                    errors.push_back(fmt::format("Entity `{}` can't include itself.", e.name));
                    continue;
                }
                auto maybeId = entities.findNonMemberByName(needName);
                if (!maybeId) {
                    errors.push_back(
                        fmt::format("Entity `{}` needs `{}` but it's missing.", e.name, needName));
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
                    auto path =
                        outputDir / ne.HeaderPath(sourcePathToTargetSubdirFn(ne.sourcePath));
                    generateds.push_back(fmt::format("\"{}\"", path));
                }
            }
        }
        return additionalNeeds;
    }
};
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

}  // namespace

std::expected<std::monostate, std::vector<std::string>> GenerateBoilerplate(
    const Project& project) {
    GeneratedFileWriter gfw(project.outputDir);
    std::vector<std::string> errors;

    // Generate empty header.
    gfw.Write(k_emptyHeaderFilename,
              fmt::format("{}\n{}\n", k_autogeneratedWarningLine, k_IntentionallyEmptyLineWarning));

    auto entityIds = project.entities.itemsWithEntities();

    TargetSubdirOfSourcePath targetSubdirOfSourcePath(project.dirConfigFiles);
    auto targetSubdirOfSourcePathFn = [&targetSubdirOfSourcePath](const fs::path& x) {
        return targetSubdirOfSourcePath.subdir(x);
    };

    flat_hash_map<std::string, std::vector<Entities::Id>> membersOfClasses;
    flat_hash_map<fs::path, fs::path, path_hash> generatedHeaderPathToSourcePath;
    for (auto id : entityIds) {
        auto& e = project.entities.entity(id);
        if (e.GetEntityKind() == EntityKind::memfn) {
            membersOfClasses[e.MemFnClassName()].push_back(id);
        } else {
            auto itb = generatedHeaderPathToSourcePath.insert(std::make_pair(
                e.HeaderPath(targetSubdirOfSourcePathFn(e.sourcePath)), e.sourcePath));
            if (!itb.second) {
                errors.push_back(
                    fmt::format("Two source files have the same generated header path. File#1: "
                                "`{}`, File#2: `{}`, generated path: `{}`",
                                itb.first->second,
                                e.sourcePath,
                                itb.first->first));
            }
        }
    }

    for (auto id : entityIds) {
        auto& e = project.entities.entity(id);
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
                IncludeSectionBuilder includes(project.outputDir, targetSubdirOfSourcePathFn);
                auto addNeedsAsHeaders =
                    [&includes, &e, &project](const std::vector<std::string>& needs) {
                        includes.addNeedsAsHeaders(project.entities, e, needs);
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
                        auto it = membersOfClasses.find(e.name);
                        if (it != membersOfClasses.end()) {
                            for (auto& mfid : it->second) {
                                auto& me = project.entities.entity(mfid);
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
                        auto it = membersOfClasses.find(e.name);
                        if (it != membersOfClasses.end()) {
                            for (auto& mfid : it->second) {
                                auto& me = project.entities.entity(mfid);
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
                    append_range(errors, std::move(renderedHeadersOr.error()));
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
                    auto it = membersOfClasses.find(e.name);
                    fs::path memberDeclarationsPath;
                    if (it == membersOfClasses.end()) {
                        memberDeclarationsPath = path_from_string(k_emptyHeaderFilename);
                    } else {
                        memberDeclarationsPath =
                            e.MemberDeclarationsPath(targetSubdirOfSourcePathFn(e.sourcePath));
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
                        gfw.Write(memberDeclarationsPath, mdContent);
                    }
                    headerContent += fmt::format("#define {} \"{}\"\n#include \"{}\"\n#undef {}\n",
                                                 k_nmtIncludeMemberDeclarationsMacro,
                                                 project.outputDir / memberDeclarationsPath,
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
            gfw.Write(e.HeaderPath(targetSubdirOfSourcePathFn(e.sourcePath)), headerContent);
        }
        std::string cppContent;
        bool cppContentProductionFailed = false;
        switch_variant(
            e.dependentProps,
            [&](const EntityDependentProperties::Fn& dp) {
                cppContent += fmt::format(
                    "{}\n#include \"{}\"\n",
                    k_autogeneratedWarningLine,
                    project.outputDir / e.HeaderPath(targetSubdirOfSourcePathFn(e.sourcePath)));
                IncludeSectionBuilder includes(project.outputDir, targetSubdirOfSourcePathFn);

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
                auto maybeMfid = project.entities.findNonMemberByName(e.MemFnClassName());
                CHECK(maybeMfid) << fmt::format("Containing class entity with name `{}` not found",
                                                e.MemFnClassName());
                auto& mfe = project.entities.entity(*maybeMfid);
                cppContent += fmt::format(
                    "{}\n#include \"{}\"\n",
                    k_autogeneratedWarningLine,
                    project.outputDir / mfe.HeaderPath(targetSubdirOfSourcePathFn(mfe.sourcePath)));
                IncludeSectionBuilder includes(project.outputDir, targetSubdirOfSourcePathFn);
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
                cppContent += fmt::format(
                    "{}\n#include \"{}\"\n",
                    k_autogeneratedWarningLine,
                    project.outputDir / e.HeaderPath(targetSubdirOfSourcePathFn(e.sourcePath)));
            });
        if (cppContentProductionFailed) {
            continue;
        }
        gfw.Write(e.DefinitionPath(), cppContent);
    }  // for a: entities
    std::ranges::sort(gfw.currentFiles);
    std::string fileListContent;
    for (auto& c : gfw.currentFiles) {
        fileListContent += fmt::format("{}\n", c);
    }
    gfw.Write(k_fileListFilename, fileListContent);
    gfw.RemoveRemainingExistingFilesAndDirs();
    if (errors.empty()) {
        return {};
    } else {
        return std::unexpected(std::move(errors));
    }
}

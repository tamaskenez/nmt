#include "nmt/GenerateBoilerplate.h"

#include "GeneratedFileWriter.h"
#include "nmt/Project.h"

namespace fs = std::filesystem;

std::expected<std::monostate, std::string> GenerateBoilerplate(const Project& project) {
    GeneratedFileWriter gfw(project.outputDir);

    // Generate empty header.
    gfw.Write(k_emptyHeaderFilename,
              fmt::format("{}\n{}\n", k_autogeneratedWarningLine, k_IntentionallyEmptyLineWarning));

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
                    content += fmt::format("{}\n", s);
                }
            }
            return content;
        }
        void addNeedsAsHeaders(const flat_hash_map<std::string, Entity>& entities,
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
            const flat_hash_map<std::string, Entity>& entities,
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
                    auto it = entities.find(std::string(needName));  // TODO(optimize): could use
                                                                     // heterogenous lookup.
                    LOG_IF(FATAL, it == entities.end()) << fmt::format(
                        "Entity `{}` needs `{}` but it's missing.", e.name, needName);
                    auto& ne = it->second;
                    if (refOnly) {
                        auto* v = ne.ForwardDeclarationNeedsOrNull();
                        LOG_IF(FATAL, v == nullptr)
                            << fmt::format("Entity `{}` needs `{}` but it's a {} and can't "
                                           "be forward declared.",
                                           e.name,
                                           need,
                                           enum_name(ne.GetEntityKind()));
                        additionalNeeds.insert(additionalNeeds.end(), BEGIN_END(*v));
                        forwardDeclarations.push_back(fmt::format("{}", ne.ForwardDeclaration()));
                    } else {
                        auto path = outputDir / ne.HeaderPath();
                        generateds.push_back(fmt::format("\"{}\"", path));
                    }
                }
            }
            return additionalNeeds;
        }
    };

    flat_hash_map<std::string, std::vector<std::string>> membersOfClasses;
    for (auto& [_, e] : project.entities) {
        if (e.GetEntityKind() == EntityKind::memfn) {
            membersOfClasses[e.MemFnClassName()].push_back(e.name);
        }
    }

    auto& entities = project.entities;

    for (auto& [_, e] : project.entities) {
        if (e.GetEntityKind() != EntityKind::memfn) {
            std::string headerContent =
                fmt::format("{}\n#pragma once\n", k_autogeneratedWarningLine);
            {
                Includes includes;
                auto addNeedsAsHeaders = [&includes, &outputDir = project.outputDir, &entities, &e](
                                             const std::vector<std::string>& needs) {
                    includes.addNeedsAsHeaders(entities, e, needs, outputDir);
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
                            for (auto& mf : it->second) {
                                auto& me = entities.at(mf);
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
                            for (auto& mf : it->second) {
                                auto& me = entities.at(mf);
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
                auto renderedHeaders = includes.render();
                if (!renderedHeaders.empty()) {
                    headerContent += fmt::format("\n{}", renderedHeaders);
                }
            }
            headerContent += "\n";
            switch (e.GetEntityKind()) {
                case EntityKind::enum_:
                case EntityKind::header:
                    headerContent += fmt::format("#include \"{}\"\n", e.path);
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
                        memberDeclarationsPath = e.MemberDeclarationsPath();
                        std::string mdContent = fmt::format("{}\n", k_autogeneratedWarningLine);
                        for (auto& mf : it->second) {
                            auto& mfe = entities.at(mf);
                            auto& dp =
                                std::get<std::to_underlying(EntityKind::memfn)>(mfe.dependentProps);
                            mdContent += fmt::format("{}\n", dp.declaration);
                        }
                        gfw.Write(memberDeclarationsPath, mdContent);
                    }
                    headerContent += fmt::format("#define {} \"{}\"\n#include \"{}\"\n#undef {}\n",
                                                 k_nmtIncludeMemberDeclarationsMacro,
                                                 project.outputDir / memberDeclarationsPath,
                                                 e.path,
                                                 k_nmtIncludeMemberDeclarationsMacro);
                } break;
                case EntityKind::memfn:
                    // memfn declaration goes to the member function declaration file which gets
                    // injected with a macro.
                    CHECK(false);
                    break;
            }

            fmt::print("Processed {} from {}\n", e.name, e.path);
            gfw.Write(e.HeaderPath(), headerContent);
        }
        std::string cppContent;
        switch_variant(
            e.dependentProps,
            [&project, &entities, &cppContent, &e](const EntityDependentProperties::Fn& dp) {
                cppContent += fmt::format("{}\n#include \"{}\"\n",
                                          k_autogeneratedWarningLine,
                                          project.outputDir / e.HeaderPath());
                Includes includes;
                includes.addNeedsAsHeaders(entities, e, dp.definitionNeeds, project.outputDir);
                auto renderedHeaders = includes.render();
                if (!renderedHeaders.empty()) {
                    cppContent += fmt::format("\n{}", renderedHeaders);
                }
                cppContent += fmt::format("\n#include \"{}\"\n", e.path);
            },
            [&project, &entities, &cppContent, &e](const EntityDependentProperties::MemFn& dp) {
                cppContent +=
                    fmt::format("{}\n#include \"{}\"\n",
                                k_autogeneratedWarningLine,
                                project.outputDir / entities.at(e.MemFnClassName()).HeaderPath());
                Includes includes;
                includes.addNeedsAsHeaders(entities, e, dp.definitionNeeds, project.outputDir);
                auto renderedHeaders = includes.render();
                if (!renderedHeaders.empty()) {
                    cppContent += fmt::format("\n{}", renderedHeaders);
                }
                cppContent += fmt::format("\n#include \"{}\"\n", e.path);
            },
            [&project, &cppContent, headerPath = e.HeaderPath()](const auto&) {
                cppContent += fmt::format("{}\n#include \"{}\"\n",
                                          k_autogeneratedWarningLine,
                                          project.outputDir / headerPath);
            });
        gfw.Write(e.DefinitionPath(), cppContent);
    }  // for a: entities
    std::ranges::sort(gfw.currentFiles);
    std::string fileListContent;
    for (auto& c : gfw.currentFiles) {
        fileListContent += fmt::format("{}\n", c);
    }
    gfw.Write(k_fileListFilename, fileListContent);
    gfw.RemoveRemainingExistingFilesAndDirs();
    return {};
}
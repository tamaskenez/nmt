#include "nmt/Project.h"

#include "ProcessSource.h"

Project::Project(std::filesystem::path outputDir)
    : outputDir(std::move(outputDir)) {}

std::expected<std::vector<std::string>, std::string> Project::AddAndProcessSource(
    const std::filesystem::path& sourcePath, bool verbose) {
    TRY_ASSIGN(processSourceResult, ProcessSource(sourcePath, verbose));

    if (processSourceResult.entity) {
        auto& entity = *processSourceResult.entity;

        auto entityKind = entity.GetEntityKind();
        std::string name = path_to_string(sourcePath.stem());
        auto itb = entities.insert(std::make_pair(name, std::move(entity)));
        if (!itb.second) {
            return std::unexpected(fmt::format("Duplicate name: `{}`, first {} then {}.",
                                               name,
                                               enum_name(itb.first->second.GetEntityKind()),
                                               enum_name(entityKind)));
        }
    }
    return std::move(processSourceResult.messages);
}

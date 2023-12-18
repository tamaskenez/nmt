#include "ProcessSource.h"

#include "ParsePreprocessedSource.h"
#include "PreprocessSource.h"
#include "ReadFile.h"

std::expected<ProcessSourceResult, std::string> ProcessSource(
    const std::filesystem::path& sourcePath, bool verbose) {
    TRY_ASSIGN_OR_UNEXPECTED(sourceContent, ReadFile(sourcePath), "Can't open file for reading");
    TRY_ASSIGN(pps, PreprocessSource(sourceContent));

    std::vector<std::string> messages;
    if (pps.specialComments.empty()) {
        if (verbose) {
            messages.push_back(
                fmt::format("Ignoring file without special comments: {}\n", sourcePath));
        }
        return ProcessSourceResult{.messages = std::move(messages)};
    }

    std::string name = path_to_string(sourcePath.stem());

    if (!sourcePath.has_parent_path()) {
        return std::unexpected(fmt::format("File {} has no parent path.", sourcePath));
    }

    std::string parentDirName = path_to_string(sourcePath.parent_path().stem());

    TRY_ASSIGN(ep, ParsePreprocessedSource(name, pps, parentDirName));

    return ProcessSourceResult{
        .entity = Entity{.name = name,
                         .sourcePath = sourcePath,
                         .visibility = ep.visibility.value_or(Entity::k_defaultVisibility),
                         .dependentProps = std::move(ep.dependentProps)},
        .messages = std::move(messages)};
}

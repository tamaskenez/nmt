#include "nmt/ProcessSource.h"

#include "ParsePreprocessedSource.h"
#include "PreprocessSource.h"
#include "ReadFile.h"

ProcessSourceResult::V ProcessSource(const std::filesystem::path& sourcePath) {
    std::error_code ec;
    auto lastWriteTime = std::filesystem::last_write_time(sourcePath, ec);
    if (ec) {
        lastWriteTime = std::filesystem::file_time_type::min();
    }
    TRY_ASSIGN_OR_RETURN_VALUE(
        sourceContent, ReadFile(sourcePath), ProcessSourceResult::CantReadFile{});
    TRY_ASSIGN_OR_RETURN_VALUE(
        pps,
        PreprocessSource(sourceContent),
        (ProcessSourceResult::Error{std::move(UNEXPECTED_ERROR), lastWriteTime}));

    if (pps.specialComments.empty()) {
        return ProcessSourceResult::SourceWithoutSpecialComments{};
    }

    std::string name = path_to_string(sourcePath.stem());

    std::optional<std::string> parentDirName;
    if (sourcePath.has_parent_path()) {
        parentDirName = path_to_string(sourcePath.parent_path().stem());
    }

    TRY_ASSIGN_OR_RETURN_VALUE(
        ep,
        ParsePreprocessedSource(name, pps, parentDirName),
        (ProcessSourceResult::Error{std::move(UNEXPECTED_ERROR), lastWriteTime}));

    return Entity{.name = name,
                  .sourcePath = sourcePath,
                  .visibility = ep.visibility.value_or(Entity::k_defaultVisibility),
                  .dependentProps = std::move(ep.dependentProps)};
}

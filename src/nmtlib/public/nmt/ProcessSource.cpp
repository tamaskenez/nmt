#include "nmt/ProcessSource.h"
#include "nmt/Project.h"

#include "ParsePreprocessedSource.h"
#include "PreprocessSource.h"
#include "ReadFile.h"

ProcessSourceResult::V ProcessSource(const std::filesystem::path& sourcePath) {
    TRY_ASSIGN_OR_RETURN_VALUE(
        sourceContent, ReadFile(sourcePath), ProcessSourceResult::CantReadFile{});
    TRY_ASSIGN_OR_RETURN_VALUE(
        pps,
        PreprocessSource(sourceContent),
        ProcessSourceResult::Error{make_vector(std::move(UNEXPECTED_ERROR))});

    if (pps.specialComments.empty()) {
        return ProcessSourceResult::SourceWithoutSpecialComments{};
    }

    if (path_to_string(sourcePath.stem()) == k_dirConfigFileName) {
        TRY_ASSIGN_OR_RETURN_VALUE(dcf,
                                   ParseDirConfigFile(pps.specialComments, sourcePath),
                                   ProcessSourceResult::Error{std::move(UNEXPECTED_ERROR)});
        return std::move(dcf);
    } else {
        TRY_ASSIGN_OR_RETURN_VALUE(ep,
                                   ParsePreprocessedSource(pps, sourcePath),
                                   (ProcessSourceResult::Error{std::move(UNEXPECTED_ERROR)}));

        return Entity{.name = ep.name,
                      .sourcePath = sourcePath,
                      .visibility = ep.visibility.value_or(Entity::k_defaultVisibility),
                      .dependentProps = std::move(ep.dependentProps)};
    }
}

std::pair<std::vector<std::string>, std::vector<std::string>> ProcessSourceAndUpdateProject(
    Project& project, Entities::Id id, bool verbose) {
    std::vector<std::string> errors, verboseMessages;
    auto& sourcePath = project.entities.sourcePath(id);
    std::error_code ec;
    auto lastWriteTime = std::filesystem::last_write_time(sourcePath, ec);
    if (ec) {
        lastWriteTime = std::filesystem::file_time_type::min();
    }
    switch_variant(
        ProcessSource(sourcePath),
        [&](Entity&& x) {
            project.entities.updateSourceWithEntity(id, std::move(x));
        },
        [&](DirConfigFile&& x) {
            project.dirConfigFiles[sourcePath] = std::move(x);
        },
        [&](ProcessSourceResult::SourceWithoutSpecialComments) {
            project.entities.updateSourceNoSpecialComments(id, lastWriteTime);
            if (verbose) {
                verboseMessages.push_back(
                    fmt::format("Ignoring file without NMT annotations: {}", sourcePath));
            }
        },
        [&](ProcessSourceResult::CantReadFile) {
            project.entities.updateSourceCantReadFile(id);
            errors.push_back(fmt::format("Can't read file: {}", sourcePath));
        },
        [&](ProcessSourceResult::Error&& x) {
            for (auto& m : x.messages) {
                errors.push_back(
                    fmt::format("Failed to process file {}, reason: {}", sourcePath, m));
            }
            project.entities.updateSourceError(id, std::move(x.messages), lastWriteTime);
        });
    return make_pair(std::move(errors), std::move(verboseMessages));
}

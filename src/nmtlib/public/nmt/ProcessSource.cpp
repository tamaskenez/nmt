#include "nmt/ProcessSource.h"
#include "nmt/Project.h"

#include "ParsePreprocessedSource.h"
#include "PreprocessSource.h"
#include "ReadFile.h"

namespace fs = std::filesystem;

ProcessSourceResult::V ProcessSource(int64_t targetId,
                                     const fs::path& targetRootSourceDir,
                                     const std::filesystem::path& sourcePath) {
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

        auto sourceRelPath =
            relativePathIfCanonicalPrefixOrNullopt(targetRootSourceDir, sourcePath);
        CHECK(sourceRelPath) << fmt::format(
            "Source path `{}` supposed to be under the target's root source directory `{}`",
            sourcePath,
            targetRootSourceDir);
        return Entity{.targetId = targetId,
                      .name = ep.name,
                      .sourcePath = sourcePath,
                      .sourceRelPath = *sourceRelPath,
                      .visibility = ep.visibility.value_or(Entity::k_defaultVisibility),
                      .dependentProps = std::move(ep.dependentProps)};
    }
}

std::pair<std::vector<std::string>, std::vector<std::string>> ProcessSourceAndUpdateProject(
    Project& project, Entities::Id id, bool verbose) {
    std::vector<std::string> errors, verboseMessages;
    auto& sourcePath = project.entities().sourcePath(id);
    std::error_code ec;
    auto lastWriteTime = std::filesystem::last_write_time(sourcePath, ec);
    if (ec) {
        lastWriteTime = std::filesystem::file_time_type::min();
    }
    auto targetId = project.entities().targetId(id);
    switch_variant(
        ProcessSource(targetId, project.targets().at(targetId).sourceDir, sourcePath),
        [&](Entity&& x) {
            project.entities_updateSourceWithEntity(id, std::move(x));
        },
        [&](DirConfigFile&& x) {
            project.dirConfigFiles()[sourcePath] = std::move(x);
        },
        [&](ProcessSourceResult::SourceWithoutSpecialComments) {
            project.entities_updateSourceNoSpecialComments(id, lastWriteTime);
            if (verbose) {
                verboseMessages.push_back(
                    fmt::format("Ignoring file without NMT annotations: {}", sourcePath));
            }
        },
        [&](ProcessSourceResult::CantReadFile) {
            project.entities_updateSourceCantReadFile(id);
            errors.push_back(fmt::format("Can't read file: {}", sourcePath));
        },
        [&](ProcessSourceResult::Error&& x) {
            for (auto& m : x.messages) {
                errors.push_back(
                    fmt::format("Failed to process file {}, reason: {}", sourcePath, m));
            }
            project.entities_updateSourceError(id, std::move(x.messages), lastWriteTime);
        });
    return make_pair(std::move(errors), std::move(verboseMessages));
}

#include "nmt/Project.h"

#include "nmt/ProcessSource.h"

namespace fs = std::filesystem;

std::expected<int64_t, std::string> Project::addTarget(std::string targetName,
                                                       fs::path sourceDir,
                                                       fs::path outputDir) {
    CHECK(!targetName.empty());
    if (auto maybeId = findTargetByName(targetName)) {
        return std::unexpected(fmt::format("Target {} already exists", targetName));
    }
    std::error_code ec;
    sourceDir = fs::canonical(sourceDir, ec);
    if (ec) {
        return std::unexpected(
            fmt::format("Can't find the canonical path of source directory `{}`, reason: {}",
                        sourceDir,
                        ec.message()));
    }
    outputDir = fs::weakly_canonical(outputDir, ec);
    if (ec) {
        return std::unexpected(
            fmt::format("Can't find the canonical path of output directory `{}`, reason: {}",
                        sourceDir,
                        ec.message()));
    }

    auto itb = targets.insert(std::make_pair(nextTargetId++,
                                             Target{.name = std::move(targetName),
                                                    .outputDir = std::move(outputDir),
                                                    .sourceDir = std::move(sourceDir)}));
    CHECK(itb.second);
    return itb.first->first;
}

std::optional<int64_t> Project::findTargetByName(std::string_view name) const {
    // TODO: could be optimized by a map, however number of targets is not expected to be high.
    for (auto& [k, v] : targets) {
        if (v.name == name) {
            return k;
        }
    }
    return std::nullopt;
}

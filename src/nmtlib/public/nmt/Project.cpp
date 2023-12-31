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

fs::path decoratedTargetNameSubdir(std::string_view targetName, Visibility visibility) {
    switch (visibility) {
        case Visibility::public_:
            return k_publicSubdir / path_from_string(targetName);
        case Visibility::private_:
            return k_privateSubdir / path_from_string(targetName);
    }
}

std::filesystem::path Project::headerPath(bool relativeToOutputDir, int64_t entityId) const {
    auto& e = entities.entity(entityId);
    auto targetIt = targets.find(e.targetId);
    CHECK(targetIt != targets.end()) << fmt::format("Target #{} not found", e.targetId);
    auto& target = targetIt->second;
    auto relPath = decoratedTargetNameSubdir(target.name, e.visibility) / e.sourceRelPath;
    return relativeToOutputDir ? relPath : target.outputDir / relPath;
}

std::filesystem::path Project::memberDeclarationsPath(bool relativeToOutputDir,
                                                      int64_t entityId) const {
    auto& e = entities.entity(entityId);

    switch (e.GetEntityKind()) {
        case EntityKind::class_:
        case EntityKind::struct_:
            break;
        case EntityKind::enum_:
        case EntityKind::fn:
        case EntityKind::header:
        case EntityKind::memfn:
            LOG(FATAL) << "Member declarations filename asked for a non-class/struct entity";
    }

    auto targetIt = targets.find(e.targetId);
    CHECK(targetIt != targets.end()) << fmt::format("Target #{} not found", e.targetId);
    auto& target = targetIt->second;

    auto relPath = e.sourceRelPath;
    // TODO check how fmt::format behaves on Windows regarding encoding, do we need to use
    // path_to_string.
    relPath.replace_filename(path_from_string(fmt::format(
        "{}{}{}", relPath.stem(), k_memberDeclarationsFilenamePostfix, relPath.extension())));
    relPath = decoratedTargetNameSubdir(target.name, e.visibility) / relPath;
    return relativeToOutputDir ? relPath : target.outputDir / relPath;
}

std::filesystem::path Project::cppPath(bool relativeToOutputDir, int64_t entityId) const {
    auto& e = entities.entity(entityId);

    auto targetIt = targets.find(e.targetId);
    CHECK(targetIt != targets.end()) << fmt::format("Target #{} not found", e.targetId);
    auto& target = targetIt->second;

    auto relPath = e.sourceRelPath;
    relPath.replace_extension(".cpp");
    relPath = decoratedTargetNameSubdir(target.name, Visibility::private_) / relPath;
    return relativeToOutputDir ? relPath : target.outputDir / relPath;
}

std::filesystem::path Project::emptyHeaderPath(bool relativeToOutputDir, int64_t targetId) const {
    auto targetIt = targets.find(targetId);
    CHECK(targetIt != targets.end()) << fmt::format("Target #{} not found", targetId);
    auto& target = targetIt->second;
    auto relPath =
        decoratedTargetNameSubdir(target.name, Visibility::public_) / k_emptyHeaderFilename;
    return relativeToOutputDir ? relPath : target.outputDir / relPath;
}

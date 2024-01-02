#include "nmt/Project.h"

#include "nmt/ProcessSource.h"

namespace fs = std::filesystem;

namespace {
fs::path structOrClassSourcePathToMemberDir(fs::path path) {
    DCHECK(!path.empty() && k_validSourceExtensions.contains(path.extension()));
    return path.replace_filename(path_from_string(
        fmt::format("{}{}", path_to_string(path.filename()), k_membersDirPostfix)));
}

bool isPathLikeMemberDir(const fs::path& path) {
    if (path.has_extension()) {
        return false;
    }
    auto filename = path_to_string(path.filename());
    return filename.size() > k_membersDirPostfix.size() && filename.ends_with(k_membersDirPostfix);
}
}  // namespace

Project::AddSourcesAndTreeItemsRecursivelyResult Project::addSourcesAndTreeItemsRecursively(
    int64_t targetId, int64_t subdirTreeItemId) {
    auto& target = _targets.at(targetId);
    auto& treeItem = _treeItems.at(subdirTreeItemId);
    CHECK(treeItem | vx::is<ProjectTreeItem::Subdir>);
    auto& subdir = std::get<ProjectTreeItem::Subdir>(treeItem);
    std::error_code ec;
    auto dit = fs::directory_iterator(subdir.sourceDir, ec);
    if (ec) {
        return AddSourcesAndTreeItemsRecursivelyResult{
            .errors = make_vector(fmt::format(
                "Can't read directory `{}`, reason: {}", subdir.sourceDir, ec.message()))};
    }
    AddSourcesAndTreeItemsRecursivelyResult result;
    for (; dit != fs::directory_iterator(); ++dit) {
        auto status = dit->status(ec);
        if (ec) {
            result.errors.push_back(fmt::format(
                "Can't get status of file `{}`, reason: {}", dit->path(), ec.message()));
            continue;
        }
        switch (status.type()) {
            using enum fs::file_type;
            case not_found:
                result.errors.push_back(
                    fmt::format("File `{}` has a type of `not_found` and that's strange, what "
                                "should we do with that? For now it's an error",
                                dit->path()));
                break;
            case regular: {
                auto ext = dit->path().extension();
                if (k_validSourceExtensions.contains(ext)) {
                    if (auto sourceIdOr =
                            _entities.addSource(targetId, target.sourceDir, dit->path())) {
                        auto childId = _nextId++;
                        _treeItems.insert(std::make_pair(
                            childId,
                            ProjectTreeItem::LeafSource{.parentTreeItem = subdirTreeItemId,
                                                        .sourceId = *sourceIdOr}));
                        subdir.children.push_back(childId);
                    } else {
                        result.errors.push_back(std::move(sourceIdOr.error()));
                    }
                } else {
                    result.verboseMessages.push_back(
                        fmt::format("Ignoring file with extension `{}`: {}", ext, dit->path()));
                }
            } break;
            case directory:
                if (!isPathLikeMemberDir(dit->path())) {
                    auto childId = _nextId++;
                    _treeItems.insert(
                        std::make_pair(childId,
                                       ProjectTreeItem::Subdir{.parentTreeItem = subdirTreeItemId,
                                                               .sourceDir = dit->path()}));
                    auto r = addSourcesAndTreeItemsRecursively(targetId, childId);
                    append_range(result.errors, std::move(r.errors));
                    append_range(result.verboseMessages, std::move(r.verboseMessages));
                }
                break;
            case none:
            case symlink:
            case block:
            case character:
            case fifo:
            case socket:
            case unknown:
                result.errors.push_back(fmt::format(
                    "File `{}` has a type of {} and it's not yet decided what to do with this type",
                    dit->path(),
                    to_string_view(status.type())));
                break;
        }
    }
    return result;
}

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

    auto targetId = _nextId++;
    auto treeItemId = _nextId++;
    auto itb = _targets.insert(std::make_pair(targetId,
                                              Target{.name = std::move(targetName),
                                                     .sourceDir = std::move(sourceDir),
                                                     .outputDir = std::move(outputDir),
                                                     .treeItem = treeItemId}));
    CHECK(itb.second);
    auto& target = itb.first->second;
    _treeItems.insert(
        std::make_pair(treeItemId,
                       ProjectTreeItem::Subdir{.targetId = targetId,
                                               .parentTreeItem = k_implicitRootTreeItemId,
                                               .sourceDir = target.sourceDir}));
    _targetsDisplayOrder.push_back(targetId);
    std::ranges::sort(_targetsDisplayOrder, {}, [this](int64_t targetId) -> const std::string& {
        return _targets.at(targetId).name;
    });
    addSourcesAndTreeItemsRecursively(targetId, treeItemId);
    return targetId;
}

std::optional<int64_t> Project::findTargetByName(std::string_view name) const {
    // TODO: could be optimized by a map, however number of targets is not expected to be high.
    for (auto& [k, v] : _targets) {
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
    auto& e = _entities.entity(entityId);
    auto targetIt = _targets.find(e.targetId);
    CHECK(targetIt != _targets.end()) << fmt::format("Target #{} not found", e.targetId);
    auto& target = targetIt->second;
    auto relPath = decoratedTargetNameSubdir(target.name, e.visibility) / e.sourceRelPath;
    return relativeToOutputDir ? relPath : target.outputDir / relPath;
}

std::filesystem::path Project::memberDeclarationsPath(bool relativeToOutputDir,
                                                      int64_t entityId) const {
    auto& e = _entities.entity(entityId);

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

    auto targetIt = _targets.find(e.targetId);
    CHECK(targetIt != _targets.end()) << fmt::format("Target #{} not found", e.targetId);
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
    auto& e = _entities.entity(entityId);

    auto targetIt = _targets.find(e.targetId);
    CHECK(targetIt != _targets.end()) << fmt::format("Target #{} not found", e.targetId);
    auto& target = targetIt->second;

    auto relPath = e.sourceRelPath;
    relPath.replace_extension(".cpp");
    relPath = decoratedTargetNameSubdir(target.name, Visibility::private_) / relPath;
    return relativeToOutputDir ? relPath : target.outputDir / relPath;
}

std::filesystem::path Project::emptyHeaderPath(bool relativeToOutputDir, int64_t targetId) const {
    auto targetIt = _targets.find(targetId);
    CHECK(targetIt != _targets.end()) << fmt::format("Target #{} not found", targetId);
    auto& target = targetIt->second;
    auto relPath =
        decoratedTargetNameSubdir(target.name, Visibility::public_) / k_emptyHeaderFilename;
    return relativeToOutputDir ? relPath : target.outputDir / relPath;
}

void Project::updateSourceInTree(int64_t id) {
    auto& source = _entities.source(id);
    auto* entity = std::get_if<Entity>(&source.state);
    bool structOrClass;
    switch (entity->GetEntityKind()) {
        case EntityKind::enum_:
        case EntityKind::fn:
        case EntityKind::header:
        case EntityKind::memfn:
            structOrClass = false;
            break;
        case EntityKind::struct_:
        case EntityKind::class_:
            structOrClass = true;
            break;
    }
    auto treeItemId = findTreeItemBySourcePath(source.targetId, source.sourcePath);
    CHECK(treeItemId) << fmt::format(
        "Target #{}, source `{}` not found in tree for update", source.targetId, source.sourcePath);
    auto& treeItem = _treeItems.at(*treeItemId);
    bool existingTreeItemIsStructOrClass = false;
    int64_t parentTreeItem = k_implicitRootTreeItemId;
    switch_variant(
        treeItem,
        [&](const ProjectTreeItem::Subdir&) {
            CHECK(false) << fmt::format("For entity #{} we found tree item #{} which happens to be "
                                        "a subdir. It should be a TreeItem holding an entity.",
                                        id,
                                        *treeItemId);
        },
        [&](const ProjectTreeItem::StructOrClass& x) {
            parentTreeItem = x.parentTreeItem;
            existingTreeItemIsStructOrClass = true;
        },
        [&](const ProjectTreeItem::LeafSource& x) {
            parentTreeItem = x.parentTreeItem;
        });
    DCHECK(parentTreeItem != k_implicitRootTreeItemId);
    if (existingTreeItemIsStructOrClass) {
        if (!structOrClass) {
            // Remove children.
            auto& structOrClassInTree = std::get<ProjectTreeItem::StructOrClass>(treeItem);
            for (auto childId : structOrClassInTree.children) {
                _treeItems.erase(childId);
            }
            structOrClassInTree.children.clear();
        }
    }
    if (structOrClass) {
        treeItem = ProjectTreeItem::StructOrClass{
            .sourceId = id,
            .parentTreeItem = parentTreeItem,
            .sourceDir = structOrClassSourcePathToMemberDir(source.sourcePath)};
    } else {
        treeItem = ProjectTreeItem::LeafSource{.parentTreeItem = parentTreeItem, .sourceId = id};
    }
}

void Project::entities_updateSourceWithEntity(int64_t id, Entity entity) {
    _entities.updateSourceWithEntity(id, std::move(entity));
    updateSourceInTree(id);
}
void Project::entities_updateSourceNoSpecialComments(
    int64_t id, std::filesystem::file_time_type lastWriteTime) {
    _entities.updateSourceNoSpecialComments(id, lastWriteTime);
    updateSourceInTree(id);
}
void Project::entities_updateSourceCantReadFile(int64_t id) {
    _entities.updateSourceCantReadFile(id);
    updateSourceInTree(id);
}
void Project::entities_updateSourceError(int64_t id,
                                         std::vector<std::string> errors,
                                         std::filesystem::file_time_type lastWriteTime) {
    _entities.updateSourceError(id, std::move(errors), lastWriteTime);
    updateSourceInTree(id);
}

std::optional<int64_t> Project::findTreeItemBySourcePath(int64_t targetId,
                                                         const fs::path& sourcePath) const {
    auto& target = _targets.at(targetId);
    if (!isCanonicalPathPrefixOfOther(target.sourceDir, sourcePath)) {
        DCHECK(false) << fmt::format(
            "Target source dir `{}` should be prefix of a source in the target `{}`",
            target.sourceDir,
            sourcePath);
        return std::nullopt;
    }

    std::function<std::optional<int64_t>(int64_t)> findInTreeItem;
    findInTreeItem = [&](int64_t treeItemId) -> std::optional<int64_t> {
        // Find tree item of target.
        auto treeItemIt = _treeItems.find(treeItemId);
        if (treeItemIt == _treeItems.end()) {
            DCHECK(false) << fmt::format(
                "Tree item #{} not found for target #{}", target.treeItem, targetId);
            return std::nullopt;
        }

        auto findHereOrInChildren =
            [&](const fs::path& sourceDir,
                const std::vector<int64_t>& children) -> std::optional<int64_t> {
            if (sourceDir == sourcePath) {
                return treeItemId;
            }
            if (!isCanonicalPathPrefixOfOther(sourceDir, sourcePath)) {
                return std::nullopt;
            }
            std::optional<int64_t> result;
            for (auto t : children) {
                if (auto r = findInTreeItem(t)) {
                    CHECK(!result);
                    result = r;
                }
            }
            return result;
        };

        return switch_variant(
            treeItemIt->second,
            [&](const ProjectTreeItem::Subdir& x) -> std::optional<int64_t> {
                return findHereOrInChildren(x.sourceDir, x.children);
            },
            [&](const ProjectTreeItem::StructOrClass& x) -> std::optional<int64_t> {
                auto& source = _entities.source(x.sourceId);
                if (source.sourcePath == sourcePath) {
                    return treeItemId;
                }
                return findHereOrInChildren(x.sourceDir, x.children);
            },
            [&](const ProjectTreeItem::LeafSource& x) -> std::optional<int64_t> {
                auto& source = _entities.source(x.sourceId);
                if (source.sourcePath == sourcePath) {
                    return treeItemId;
                }
                return std::nullopt;
            });
    };
    return findInTreeItem(target.treeItem);
}

/*
namespace {
int64_t parentTreeItem(const ProjectTreeItem::V& x) {
    return switch_variant(
        x,
        [](const ProjectTreeItem::Subdir& x) {
            return x.parentTreeItem;
        },
        [](const ProjectTreeItem::StructOrClass& x) {
            return x.parentTreeItem;
        },
        [&](const ProjectTreeItem::LeafSource& x) {
            return x.parentTreeItem;
        });
}
}  // namespace
*/
/*
void Project::eraseTreeItem(int64_t parentId, int64_t childId) {
    auto& parent = _treeItems.at(parentId);
    switch_variant(
        parent,
        [](const ProjectTreeItem::Subdir& x) {
            auto it = std::ranges::find(x.children, childId);
            CHECK(it != x.children.end());
            x.children.erase(it);
        },
        [](const ProjectTreeItem::StructOrClass& x) {},
        [&](const ProjectTreeItem::LeafEntity& x) {
            auto parent = x.parentTreeItem;
            _treeItems.erase(it);
            _treeItems.at(parent).eraseChild(treeItemId);
        });
}

void Project::removeEntityFromTree(int64_t id) {
    auto& source = _entities.source(id);
    if (auto treeItemId = findTreeItemBySourcePath(source.targetId, source.sourcePath)) {
        auto it = _treeItems.find(*treeItemId);
        CHECK(it != _treeItems.end());
        eraseTreeItem(parentTreeItem(*treeItemId), *treeItemId);
    }
}

void Project::addEntityToTree(int64_t id) {}
*/

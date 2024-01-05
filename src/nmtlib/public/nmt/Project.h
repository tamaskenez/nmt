#pragma once

#include "nmt/DirConfigFile.h"
#include "nmt/Entities.h"
#include "nmt/Entity.h"
#include "nmt/base_types.h"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ProjectTreeItem {
struct Subdir {
    std::optional<int64_t> targetId;
    int64_t parentTreeItem;
    std::filesystem::path sourceDir;
    std::vector<int64_t> children;  // tree item id.
};
struct StructOrClass {
    int64_t sourceId;
    int64_t parentTreeItem;
    std::filesystem::path sourceDir;
    std::vector<int64_t> children;  // tree item id.
};
struct LeafSource {
    int64_t parentTreeItem;
    int64_t sourceId;
};
using V = std::variant<Subdir, StructOrClass, LeafSource>;
}  // namespace ProjectTreeItem

struct Project {
    using DirConfigFiles = flat_hash_map<std::filesystem::path, DirConfigFile, path_hash>;
    struct Target {
        std::string name;
        std::filesystem::path sourceDir, outputDir;
        int64_t treeItem;
    };

    const Entities& entities() const {
        return _entities;
    }
    const flat_hash_map<int64_t, Target>& targets() const {
        return _targets;
    }
    DirConfigFiles& dirConfigFiles() {
        return _dirConfigFiles;
    }

    struct AddTargetResult {
        int64_t targetId;
        std::vector<std::string> nonFatalErrors, verboseMessages;
    };
    /// Glob-recurse `sourceDir`, add sources and full tree for target.
    [[nodiscard]] std::expected<AddTargetResult, std::string> addTarget(
        std::string targetName, std::filesystem::path sourceDir, std::filesystem::path outputDir);
    /// Return: target id
    std::optional<int64_t> findTargetByName(std::string_view name) const;

    std::filesystem::path headerPath(bool relativeToOutputDir, int64_t entityId) const;
    std::filesystem::path cppPath(bool relativeToOutputDir, int64_t entityId) const;
    std::filesystem::path memberDeclarationsPath(bool relativeToOutputDir, int64_t entityId) const;
    std::filesystem::path emptyHeaderPath(bool relativeToOutputDir, int64_t targetId) const;

    /// It's an error if `id` doesn't exist.
    void entities_updateSourceWithEntity(int64_t id, Entity entity);
    /// It's an error if `id` doesn't exist.
    void entities_updateSourceNoSpecialComments(int64_t id,
                                                std::filesystem::file_time_type lastWriteTime);
    /// It's an error if `id` doesn't exist.
    void entities_updateSourceCantReadFile(int64_t id);
    /// It's an error if `id` doesn't exist.
    void entities_updateSourceError(int64_t id,
                                    std::vector<std::string> errors,
                                    std::filesystem::file_time_type lastWriteTime);

   private:
    static constexpr int64_t k_implicitRootTreeItemId = -1;
    int64_t _nextId = 1;
    node_hash_map<int64_t, Target>
        _targets;  // Not flat_hash_map: small size, not worth to optimize.
    std::vector<int64_t> _targetsDisplayOrder;
    node_hash_map<int64_t, ProjectTreeItem::V>
        _treeItems;  // Not flat_hash_map: tree item might be held while new ones are created.

    Entities _entities;
    DirConfigFiles _dirConfigFiles;

    // void removeEntityFromTree(int64_t id);
    // void addEntityToTree(int64_t id);
    std::optional<int64_t> findTreeItemBySourcePath(int64_t targetId,
                                                    const std::filesystem::path& sourcePath) const;
    // void eraseTreeItem(int64_t parentId, int64_t childId);
    void updateSourceInTree(int64_t id);

    struct AddSourcesAndTreeItemsRecursivelyResult {
        std::vector<std::string> errors, verboseMessages;
    };
    /// Return errors during directory traversal but try reading everything.
    [[nodiscard]] AddSourcesAndTreeItemsRecursivelyResult addSourcesAndTreeItemsRecursively(
        int64_t targetId, int64_t subdirTreeItemId);

    struct AddSourcesFromMemberDirResult {
        std::vector<int64_t> children;
        std::vector<std::string> errors, verboseMessages;
    };
    AddSourcesFromMemberDirResult addSourcesFromMemberDir(int64_t targetId,
                                                          const std::filesystem::path& memberDir,
                                                          int64_t structClassTreeItemId);
};

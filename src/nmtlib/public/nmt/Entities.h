#pragma once

#include "nmt/Entity.h"
#include "nmt/base_types.h"

#include <expected>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace EntitiesItemState {
struct CantGetCanonicalPath {
    std::string message;
};
struct NewSource {};
struct CantReadFile {};
struct SourceWithoutSpecialComments {
    std::filesystem::file_time_type lastWriteTime;
};
struct Error {
    std::vector<std::string> messages;
    std::filesystem::file_time_type lastWriteTime;
};
using V = std::variant<NewSource, CantReadFile, SourceWithoutSpecialComments, Error, Entity>;
}  // namespace EntitiesItemState

class Entities {
   public:
    using Id = int64_t;
    struct Item {
        int64_t targetId;
        const std::filesystem::path& sourcePath;  // points to sourcePathToId keys
        EntitiesItemState::V state;
    };

    /// Adding a source which has already been added is an error in Debug, ignored in Release.
    [[nodiscard]] std::expected<int64_t, std::string> addSource(
        int64_t targetId,
        const std::filesystem::path& targetSourceDir,
        const std::filesystem::path& path);

    /// Return out-of-date sources and sources with errors, sorted by sourcePath.
    std::vector<Id> dirtySources() const;

    /// Return sources with valid entities.
    std::vector<Id> entities() const;

    /// It's an error if there's no entity for `id` (either because `id` doesn't exist or the item
    /// at `id` has no entity)
    const Entity& entity(Id id) const;

    /// It's an error if `id` doesn't exist.
    const Entities::Item& source(Id id) const;

    /// It's an error if there's no item for`id`.
    const std::filesystem::path& sourcePath(Id id) const;

    /// It's an error if there's no item for`id`.
    int64_t targetId(Id id) const;

    std::vector<Id> itemsWithEntities() const;
    std::optional<Id> findNonMemberByName(int64_t targetId, std::string_view name) const;
    std::optional<Id> findEntityBySourcePath(int64_t targetId,
                                             const std::filesystem::path& p) const;

    /// It's an error if `id` doesn't exist.
    void updateSourceWithEntity(Id id, Entity entity);
    /// It's an error if `id` doesn't exist.
    void updateSourceNoSpecialComments(Id id, std::filesystem::file_time_type lastWriteTime);
    /// It's an error if `id` doesn't exist.
    void updateSourceCantReadFile(Id id);
    /// It's an error if `id` doesn't exist.
    void updateSourceError(Id id,
                           std::vector<std::string> errors,
                           std::filesystem::file_time_type lastWriteTime);

   private:
    flat_hash_map<Id, Item> items;
    node_hash_map<std::filesystem::path, Id, path_hash>
        sourcePathToId;  // Keys must have stable addresses, Item::sourcePath is a reference to the
                         // key.
    Id nextId = 1;
};

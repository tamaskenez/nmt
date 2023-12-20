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
    std::string message;
    std::filesystem::file_time_type lastWriteTime;
};
using V = std::variant<NewSource, CantReadFile, SourceWithoutSpecialComments, Error, Entity>;
}  // namespace EntitiesItemState

class Entities {
   public:
    using Id = int64_t;
    /// Adding a source which has already been added is an error in Debug, ignored in Release.
    std::expected<std::monostate, std::string> addSource(const std::filesystem::path& path);
    /// Return out-of-date sources and sources with errors, sorted by sourcePath.
    std::vector<Id> dirtySources() const;
    std::vector<Id> entities() const;
    /// It's an error if there's no entity for `id` (either because `id` doesn't exist or the item
    /// at `id` has no entity)
    const Entity& entity(Id id) const;
    /// It's an error if there's no item for`id`.
    const std::filesystem::path& sourcePath(Id id) const;
    std::vector<Id> itemsWithEntities() const;
    std::optional<Id> findByName(std::string_view name) const;
    /// It's an error if `id` doesn't exist.
    void updateSourceWithEntity(Id id, Entity entity);
    void updateSourceNoSpecialComments(Id id, std::filesystem::file_time_type lastWriteTime);
    void updateSourceCantReadFile(Id id);
    void updateSourceError(Id id,
                           std::string message,
                           std::filesystem::file_time_type lastWriteTime);

   private:
    struct Item {
        const std::filesystem::path& sourcePath;  // points to sourcePathToId keys
        EntitiesItemState::V state;
    };
    flat_hash_map<Id, Item> items;
    node_hash_map<std::filesystem::path, Id, path_hash>
        sourcePathToId;  // Keys must have stable addresses, Item.sourcePath is a reference to the
                         // key.
    Id nextId = 1;
};

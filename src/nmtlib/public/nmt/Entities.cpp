#include "nmt/Entities.h"

namespace fs = std::filesystem;
namespace ItemState = EntitiesItemState;
using namespace vx;

std::expected<int64_t, std::string> Entities::addSource(int64_t targetId,
                                                        const fs::path& targetSourceDir,
                                                        const std::filesystem::path& path) {
    std::error_code ec;
    auto canonicalPath = fs::weakly_canonical(path, ec);
    if (ec) {
        return std::unexpected(fmt::format("Can't get canonical path: {}", ec.message()));
    }
    CHECK(isCanonicalPathPrefixOfOther(targetSourceDir, path));
    const auto id = nextId++;
    auto itb = sourcePathToId.insert(std::make_pair(std::move(canonicalPath), id));
    if (itb.second) {
        const auto& sourcePath = itb.first->first;
        items.insert(std::make_pair(id, Item{targetId, sourcePath, ItemState::NewSource{}}));
    } else {
        DCHECK(itb.second) << fmt::format("Duplicated source: {}", path);
    }

    return id;
}

std::vector<Entities::Id> Entities::dirtySources() const {
    std::vector<Id> ids;
    ids.reserve(items.size());
    for (auto& [k, v] : items) {
        if (switch_variant(
                v.state,
                [](ItemState::NewSource) {
                    return true;
                },
                [](ItemState::CantReadFile) {
                    return true;
                },
                [&v](const ItemState::SourceWithoutSpecialComments& x) {
                    std::error_code ec;
                    auto lastWriteTime = fs::last_write_time(v.sourcePath, ec);
                    return ec || lastWriteTime > x.lastWriteTime;
                },
                [&v](const ItemState::Error& x) {
                    std::error_code ec;
                    auto lastWriteTime = fs::last_write_time(v.sourcePath, ec);
                    return ec || lastWriteTime > x.lastWriteTime;
                },
                [&v](const Entity& x) {
                    std::error_code ec;
                    auto lastWriteTime = fs::last_write_time(v.sourcePath, ec);
                    return ec || lastWriteTime > x.lastWriteTime;
                })) {
            ids.push_back(k);
        }
    }
    std::ranges::sort(ids, {}, [this](Id x) -> const fs::path& {
        return items.at(x).sourcePath;
    });
    return ids;
}

std::vector<Entities::Id> Entities::entities() const {
    std::vector<Id> ids;
    ids.reserve(items.size());
    for (auto& [k, v] : items) {
        if (v.state | is<Entity>) {
            ids.push_back(k);
        }
    }
    return ids;
}

std::string_view itemStateName(const ItemState::V& x) {
    return switch_variant(
        x,
#define CASE(X, Y)                     \
    [](const X&) -> std::string_view { \
        return #Y;                     \
    }
        CASE(ItemState::NewSource, NewSource),
        CASE(ItemState::CantReadFile, CantReadFile),
        CASE(ItemState::SourceWithoutSpecialComments, SourceWithoutSpecialComments),
        CASE(ItemState::Error, Error),
        CASE(Entity, Entity)
#undef CASE
    );
}

const Entity& Entities::entity(Id id) const {
    auto it = items.find(id);
    CHECK(it != items.end()) << fmt::format("Item#{} not found", id);
    CHECK(it->second.state | is<Entity>) << fmt::format(
        "Item#{} has no entity, it's state is: {}", id, itemStateName(it->second.state));
    return std::get<Entity>(it->second.state);
}

const std::filesystem::path& Entities::sourcePath(Id id) const {
    auto it = items.find(id);
    CHECK(it != items.end()) << fmt::format("Item#{} not found", id);
    return it->second.sourcePath;
}

int64_t Entities::targetId(Id id) const {
    auto it = items.find(id);
    CHECK(it != items.end()) << fmt::format("Item#{} not found", id);
    return it->second.targetId;
}

std::vector<Entities::Id> Entities::itemsWithEntities() const {
    std::vector<Id> ids;
    ids.reserve(items.size());
    for (auto& [k, v] : items) {
        if (v.state | is<Entity>) {
            ids.push_back(k);
        }
    }
    return ids;
}

std::optional<Entities::Id> Entities::findNonMemberByName(int64_t targetId,
                                                          std::string_view name) const {
    // TODO add lookup table after namespace has been implemented.
    std::optional<Id> maybeId;
    for (auto& [k, v] : items) {
        if (auto* e = std::get_if<Entity>(&v.state); e && e->name == name && e->targetId == targetId
                                                     && !isMemberEntityKind(e->GetEntityKind())) {
            CHECK(!maybeId) << fmt::format(
                "Two entities (#{} and #{}) with the same name: {}", *maybeId, k, name);
            maybeId = k;
        }
    }
    return maybeId;
}

std::optional<Entities::Id> Entities::findEntityBySourcePath(int64_t targetId,
                                                             const std::filesystem::path& p) const {
    // TODO add lookup table after namespace has been implemented.
    std::optional<Id> maybeId;
    for (auto& [k, v] : items) {
        if (auto* e = std::get_if<Entity>(&v.state);
            e && e->sourcePath == p && e->targetId == targetId) {
            CHECK(!maybeId) << fmt::format(
                "Two entities (#{} and #{}) with the same sourcePath: {}", *maybeId, k, p);
            maybeId = k;
        }
    }
    return maybeId;
}

void Entities::updateSourceWithEntity(Id id, Entity entity) {
    auto it = items.find(id);
    CHECK(it != items.end()) << fmt::format("Item id #{} doesn't exist", id);
    it->second.state = std::move(entity);
}

void Entities::updateSourceNoSpecialComments(Id id, std::filesystem::file_time_type lastWriteTime) {
    auto it = items.find(id);
    CHECK(it != items.end()) << fmt::format("Item id #{} doesn't exist", id);
    it->second.state = ItemState::SourceWithoutSpecialComments{lastWriteTime};
}

void Entities::updateSourceCantReadFile(Id id) {
    auto it = items.find(id);
    CHECK(it != items.end()) << fmt::format("Item id #{} doesn't exist", id);
    it->second.state = ItemState::CantReadFile{};
}

void Entities::updateSourceError(Id id,
                                 std::vector<std::string> errors,
                                 std::filesystem::file_time_type lastWriteTime) {
    auto it = items.find(id);
    CHECK(it != items.end()) << fmt::format("Item id #{} doesn't exist", id);
    it->second.state = ItemState::Error{std::move(errors), lastWriteTime};
}

const Entities::Item& Entities::source(Id id) const {
    auto it = items.find(id);
    CHECK(it != items.end()) << fmt::format("Source #{} doesn't exist", id);
    return it->second;
}

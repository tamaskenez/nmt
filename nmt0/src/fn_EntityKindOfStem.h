// needs: <optional>, <string>, EntityKind*, <string_view>
std::optional<EntityKind> EntityKindOfStem(std::string_view stem)
// needs: EntityKind, <unordered_map>
{
    auto firstUnderscorePos = stem.find('_');
    if (firstUnderscorePos == std::string::npos) {
        return std::nullopt;
    }
    static const std::unordered_map<std::string_view, EntityKind> m = {
        {"fn", EntityKind::fn},
        {"sc", EntityKind::sc},
        {"enum", EntityKind::enum_},
    };

    // Force completeness of map.
    [[maybe_unused]] auto _ = [](EntityKind ek) {
        switch (ek) {
            case EntityKind::enum_:
            case EntityKind::fn:
            case EntityKind::sc:
                break;
        }
    };

    auto it = m.find(stem.substr(0, firstUnderscorePos));
    if (it == m.end()) {
        return std::nullopt;
    }
    return it->second;
}

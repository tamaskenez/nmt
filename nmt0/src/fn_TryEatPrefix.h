// needs: <optional>, <string_view>
std::optional<std::string_view> TryEatPrefix(std::string_view sv, std::string_view prefix) {
    if (sv.starts_with(prefix)) {
        sv.remove_prefix(prefix.size());
        return sv;
    }
    return std::nullopt;
}

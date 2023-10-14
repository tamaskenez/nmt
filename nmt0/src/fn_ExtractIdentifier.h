// needs: <string_view>, <optional>
std::optional<std::string_view> ExtractIdentifier(std::string_view sv)
// needs: Trim, <cctype>
{
    auto r = Trim(sv);
    if (r.empty()) {
        return std::nullopt;
    }
    if (isdigit(r.front())) {
        return std::nullopt;
    }
    for (auto c : r) {
        if (!isalnum(c) && c != '_') {
            return std::nullopt;
        }
    }
    return r;
}

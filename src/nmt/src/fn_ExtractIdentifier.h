#pragma once

std::optional<std::string_view> ExtractIdentifier(std::string_view sv) {
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

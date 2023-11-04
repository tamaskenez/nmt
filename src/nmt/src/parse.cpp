#include "pch.h"

#include "parse.h"

std::string_view EatBlank(std::string_view sv) {
    while (!sv.empty() && isblank(sv.front())) {
        sv.remove_prefix(1);
    }
    return sv;
}

std::string_view EatSpace(std::string_view sv) {
    while (!sv.empty() && isspace(sv.front())) {
        sv.remove_prefix(1);
    }
    return sv;
}

std::string_view EatWhileNot(std::string_view sv, std::string_view chs) {
    while (!sv.empty() && chs.find(sv.front()) == std::string_view::npos) {
        sv.remove_prefix(1);
    }
    return sv;
}

std::optional<std::pair<std::string_view, std::string_view>> TryEatCSymbol(std::string_view sv) {
    if (sv.empty() || !(isalpha(sv.front()) || sv.front() == '_')) {
        return std::nullopt;
    }
    auto sv0 = sv;
    sv.remove_prefix(1);
    while (!sv.empty() && (isalnum(sv.front()) || sv.front() == '_')) {
        sv.remove_prefix(1);
    }
    return std::make_pair(sv0.substr(0, sv0.size() - sv.size()), sv);
}

std::optional<std::string_view> TryEatPrefix(std::string_view sv, std::string_view prefix) {
    if (sv.starts_with(prefix)) {
        sv.remove_prefix(prefix.size());
        return sv;
    }
    return std::nullopt;
}

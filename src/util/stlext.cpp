#include "util/stlext.h"

#include <cctype>

std::string_view trim(std::string_view sv) {
    while (!sv.empty() && isspace(sv.front())) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && isspace(sv.back())) {
        sv.remove_suffix(1);
    }
    return sv;
}

std::string path_to_string(const std::filesystem::path& p) {
    auto u8string = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8string.data()), u8string.size());
}

std::filesystem::path path_from_string(std::string_view sv) {
    auto p = reinterpret_cast<const char8_t*>(sv.data());
    return std::filesystem::path(p, p + sv.size());
}

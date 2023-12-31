#include "util/stlext.h"

#include <cctype>

namespace fs = std::filesystem;

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

bool isCanonicalPathPrefixOfOther(const std::filesystem::path& parent,
                                  const std::filesystem::path& child) {
    auto mr = std::ranges::mismatch(parent, child);
    return mr.in1 == parent.end() && mr.in2 != child.end();
}

std::optional<fs::path> relativePathIfCanonicalPrefixOrNullopt(const std::filesystem::path& parent,
                                                               const std::filesystem::path& child) {
    auto mr = std::ranges::mismatch(parent, child);
    if (mr.in1 == parent.end() && mr.in2 != child.end()) {
        fs::path result;
        for (auto it = mr.in2; it != child.end(); ++it) {
            result /= *it;
        }
        return result;
    }
    return std::nullopt;
}

/*
std::optional<std::filesystem::path> parent_dirname(const std::filesystem::path& p) {
    auto it = p.end();
    if (it == p.begin()) {
        return std::nullopt;
    }
    --it;
    if (it == p.begin()) {
        return std::nullopt;
    }
    return *it;
}
*/

#include "nmtutil.h"

#include "nmt/constants.h"

namespace fs = std::filesystem;

fs::path structOrClassSourcePathToMemberDir(fs::path path) {
    DCHECK(!path.empty() && k_validSourceExtensions.contains(path.extension()));
    return path.replace_filename(
        path_from_string(fmt::format("{}{}", path_to_string(path.stem()), k_membersDirPostfix)));
}

std::optional<std::string_view> extractContainingStructOrClassNameFromMemberDirName(
    std::string_view filename) {
    if (filename.size() <= k_membersDirPostfix.size() || !filename.ends_with(k_membersDirPostfix)) {
        return std::nullopt;
    }
    auto result = filename.substr(0, filename.size() - k_membersDirPostfix.size());
    if (isCIdentifier(result)) {
        return result;
    }
    return std::nullopt;
}

bool isPathLikeMemberDir(const fs::path& path) {
    return extractContainingStructOrClassNameFromMemberDirName(path_to_string(path.filename()))
        .has_value();
}

std::optional<std::string> extractContainingStructOrClassNameFromMemberDir(const fs::path& path) {
    if (auto r =
            extractContainingStructOrClassNameFromMemberDirName(path_to_string(path.filename()))) {
        return std::string(*r);
    }
    return std::nullopt;
}

bool isCIdentifier(std::string_view sv) {
    if (sv.empty() || isdigit(sv.front())) {
        return false;
    }
    for (auto c : sv) {
        if (!isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

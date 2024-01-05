#pragma once

std::filesystem::path structOrClassSourcePathToMemberDir(std::filesystem::path path);
bool isPathLikeMemberDir(const std::filesystem::path& path);
std::optional<std::string> extractContainingStructOrClassNameFromMemberDir(
    const std::filesystem::path& path);
bool isCIdentifier(std::string_view sv);
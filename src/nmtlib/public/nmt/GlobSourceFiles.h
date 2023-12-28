#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

struct GlobSourceFilesResult {
    std::vector<std::filesystem::path> sources;
    std::vector<std::string> messages;
};

std::expected<GlobSourceFilesResult, std::vector<std::string>> GlobSourceFiles(
    const std::filesystem::path& sourceDir, bool verbose);

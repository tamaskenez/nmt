#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

struct ResolveSourcesFromCommandLineResult {
    std::vector<std::filesystem::path> resolvedSources;
    std::vector<std::string> messages;
};

std::expected<ResolveSourcesFromCommandLineResult, std::string> ResolveSourcesFromCommandLine(
    std::vector<std::filesystem::path> sourcesFromCommandLine, bool verbose);

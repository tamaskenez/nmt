#pragma once

#include <expected>
#include <filesystem>
#include <vector>

struct ProgramOptions {
    bool verbose = false;
    std::vector<std::filesystem::path> sources;
    std::filesystem::path outputDir;
};

std::expected<ProgramOptions, int> ParseProgramOptions(int argc, char* argv[]);

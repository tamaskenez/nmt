#pragma once

#include <expected>
#include <filesystem>
#include <vector>

struct ProgramOptions {
    bool verbose = false;
    std::filesystem::path sourceDir;
    std::filesystem::path outputDir;
    std::string target;
};

std::expected<ProgramOptions, int> ParseProgramOptions(int argc, char* argv[]);

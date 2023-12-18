#pragma once

#include "nmt/Entity.h"
#include "nmt/base_types.h"

#include <expected>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

struct Project {
    std::filesystem::path outputDir;
    flat_hash_map<std::string, Entity> entities;

    explicit Project(std::filesystem::path outputDir);
    std::expected<std::vector<std::string>, std::string> AddAndProcessSource(
        const std::filesystem::path& sourcePath, bool verbose);
    std::expected<std::vector<std::string>, std::string> AddAndProcessSources(
        const std::vector<std::filesystem::path>& sources, bool verbose);
};

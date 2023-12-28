#pragma once

#include "nmt/DirConfigFile.h"
#include "nmt/Entities.h"
#include "nmt/Entity.h"
#include "nmt/base_types.h"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

struct Project {
    struct Target {
        std::string name;
        std::filesystem::path sourceDir, outputDir;
    };
    int64_t nextTargetId = 1;
    flat_hash_map<int64_t, Target> targets;

    Entities entities;
    flat_hash_map<std::filesystem::path, DirConfigFile, path_hash> dirConfigFiles;

    /// Return: target id
    std::expected<int64_t, std::string> addTarget(std::string targetName,
                                                  std::filesystem::path sourceDir,
                                                  std::filesystem::path outputDir);
    /// Return: target id
    std::optional<int64_t> findTargetByName(std::string_view name) const;
};

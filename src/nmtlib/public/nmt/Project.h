#pragma once

#include "nmt/DirConfigFile.h"
#include "nmt/Entities.h"
#include "nmt/Entity.h"
#include "nmt/base_types.h"

#include <expected>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

struct Project {
    std::filesystem::path outputDir;
    Entities entities;
    flat_hash_map<std::filesystem::path, DirConfigFile, path_hash> dirConfigFiles;

    explicit Project(std::filesystem::path outputDir);
};

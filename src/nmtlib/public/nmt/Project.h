#pragma once

#include "nmt/Entities.h"
#include "nmt/Entity.h"

#include <expected>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

struct Project {
    std::filesystem::path outputDir;
    Entities entities;

    explicit Project(std::filesystem::path outputDir);
};

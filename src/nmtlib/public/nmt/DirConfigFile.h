#pragma once

#include <filesystem>
#include <optional>

struct DirConfigFile {
    std::filesystem::path parentDir;
    std::optional<std::filesystem::path> subdir;
};

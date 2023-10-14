#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

std::optional<std::vector<std::string>> ReadFileAsLines(const std::filesystem::path& p);

#pragma once

#include <filesystem>
#include <optional>
#include <string>

std::optional<std::string> ReadFile(const std::filesystem::path& p);

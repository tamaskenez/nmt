#pragma once

#include <filesystem>
#include <string_view>

[[nodiscard]] bool WriteFile(const std::filesystem::path& p, std::string_view content);

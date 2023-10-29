#pragma once

[[nodiscard]] bool WriteFile(const std::filesystem::path& p, std::string_view content);

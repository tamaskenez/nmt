#pragma once

#include <expected>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

std::expected<std::optional<std::pair<std::string_view, std::vector<std::string>>>, std::string>
TryEatSpecialComment(std::string_view sv);

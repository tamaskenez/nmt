#pragma once

#include <nonstd/expected.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

nonstd::expected<std::optional<std::pair<std::string_view, std::vector<std::string>>>, std::string>
TryEatSpecialComment(std::string_view sv);
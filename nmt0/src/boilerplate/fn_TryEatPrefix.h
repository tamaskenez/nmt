#pragma once

#include <optional>
#include <string_view>

std::optional<std::string_view> TryEatPrefix(std::string_view sv, std::string_view prefix);
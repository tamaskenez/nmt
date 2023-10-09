#pragma once

#include "sc_SpecialComment.h"

#include <nonstd/expected.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

nonstd::expected<std::optional<std::pair<std::string_view, SpecialComment>>, std::string>
TryEatSpecialComment(std::string_view sv);

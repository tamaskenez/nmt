#pragma once

#include "data.h"

std::expected<std::pair<SpecialComment, std::string_view>, std::string> TryEatSpecialComment(
    std::string_view sv);

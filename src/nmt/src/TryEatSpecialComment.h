#pragma once

#include "data.h"

std::expected<SpecialComment, std::string> TryEatSpecialComment(std::string_view sv);

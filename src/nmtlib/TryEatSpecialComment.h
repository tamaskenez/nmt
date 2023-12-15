#pragma once

#include "data.h"

std::expected<SpecialComment, std::string> TryEatSpecialCommentAfterSlashSlash(
    std::string_view sv, bool previousShouldContinue);

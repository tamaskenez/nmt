#pragma once

#include "SpecialComment.h"

struct PreprocessedSource {
    std::vector<SpecialComment> specialComments;
    std::vector<std::string_view> nonCommentCode;
};

std::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv);

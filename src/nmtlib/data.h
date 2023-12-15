#pragma once

#include "nmt/enums.h"

#include "libtokenizer.h"
#include "nmt/base_types.h"
#include "nmt/constants.h"

struct SpecialComment {
    std::string_view keyword;  // Points into the original source text.
    std::vector<std::string_view> list;
    bool trailingComma =
        false;  // Used only when parsing, one specialcomment can continue in the next line.
};
struct PreprocessedSource {
    std::vector<SpecialComment> specialComments;
    libtokenizer::Result tokens;
};

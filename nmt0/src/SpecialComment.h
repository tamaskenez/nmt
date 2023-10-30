#pragma once

struct SpecialComment {
    std::string_view keyword;
    std::vector<std::string_view> list;
};

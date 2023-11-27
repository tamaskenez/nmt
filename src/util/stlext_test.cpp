#include "util/stlext.h"

#include <gtest/gtest.h>

TEST(stlext, path_to_string_and_back) {
    char8_t input_utf8[] = {
        // clang-format off
        0x24,                   // U+0024: `$`
        0xC2, 0xA3,             // U+00A3: `£`
        0xD0, 0x98,             // U+0418: `И`
        0xE0, 0xA4, 0xB9,       // U+0939: `ह`
        0xE2, 0x82, 0xAC,       // U+20AC: `€`
        0xED, 0x95, 0x9C,       // U+D55C: `한`
        0xF0, 0x90, 0x8D, 0x88, // U+10348: `𐍈`
		0
        // clang-format on
    };
    auto input_u8string_view = std::u8string_view(input_utf8);
    auto input_string_view = std::string_view(reinterpret_cast<const char*>(input_utf8));

    auto path = path_from_string(input_string_view);
    ASSERT_EQ(path.u8string(), input_u8string_view);

    auto string = path_to_string(path);
    ASSERT_EQ(string, input_string_view);
}

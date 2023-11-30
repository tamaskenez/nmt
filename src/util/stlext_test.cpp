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

TEST(stlext, append_range_copy) {
    const std::vector<int> u = {1, 2, 3};
    std::vector<int> v = {4, 5};
    append_range(v, u);
    ASSERT_EQ(v, std::vector<int>({4, 5, 1, 2, 3}));
}

struct MoveOnly {
    MoveOnly() = default;
    explicit MoveOnly(int i)
        : i(i) {}
    MoveOnly(const MoveOnly&) = default;
    MoveOnly(MoveOnly&& y)
        : i(y.i) {
        y.i += 100;
    }
    MoveOnly& operator=(const MoveOnly&) = default;
    MoveOnly& operator=(MoveOnly&& y) {
        i = y.i;
        y.i += 100;
        return *this;
    }
    bool operator<=>(const MoveOnly&) const = default;
    int i = 0;
};

TEST(stlext, append_range_move) {
    std::vector<MoveOnly> u;
    u.push_back(MoveOnly{1});
    u.push_back(MoveOnly{2});
    u.push_back(MoveOnly{3});
    std::vector<MoveOnly> v;
    v.push_back(MoveOnly{4});
    v.push_back(MoveOnly{5});
    append_range(v, std::move(u));
    ASSERT_EQ(
        v,
        std::vector<MoveOnly>({MoveOnly{4}, MoveOnly{5}, MoveOnly{1}, MoveOnly{2}, MoveOnly{3}}));
    ASSERT_EQ(u, std::vector<MoveOnly>({MoveOnly{101}, MoveOnly{102}, MoveOnly{103}}));
}

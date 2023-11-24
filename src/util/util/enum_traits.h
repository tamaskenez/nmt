#pragma once

#include <absl/log/check.h>

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

// For usage, see enum_traits.cpp

template<typename Enum>
struct enum_traits {};

template<typename Enum>
constexpr bool is_enum_with_trivial_values() {
    using Traits = enum_traits<Enum>;
    for (size_t i = 0; i < Traits::elements.size(); ++i) {
        if (std::cmp_not_equal(std::to_underlying(Traits::elements[i]), i)) {
            return false;
        }
    }
    return true;
};

template<typename Enum>
constexpr std::string_view enum_name(Enum e) noexcept {
    using Traits = enum_traits<Enum>;
    static_assert(Traits::elements.size() == Traits::names.size());

    size_t idx{};

    if constexpr (is_enum_with_trivial_values<Enum>()) {
        idx = size_t(std::to_underlying(e));
    } else {
        auto it = std::find_if(Traits::elements.begin(), Traits::elements.end(), [e](Enum ei) {
            return ei == e;
        });
        ASSERT(it != Traits::elements.end());
        idx = it - Traits::elements.begin();
    }
    CHECK(0 <= idx && idx < Traits::names.size());
    return Traits::names[idx];
}

template<typename Enum>
constexpr std::optional<Enum> enum_from_name(std::string_view name) {
    using Traits = enum_traits<Enum>;
    for (auto it = Traits::names.begin(); it != Traits::names.end(); ++it) {
        if (*it == name) {
            auto idx = it - Traits::names.begin();
            CHECK(0 <= idx && std::cmp_less(idx, Traits::names.size()));
            return Traits::elements[size_t(idx)];
        }
    }
    return std::nullopt;
}

template<typename Enum>
constexpr size_t enum_size() noexcept {
    return enum_traits<Enum>::elements.size();
}

template<typename Enum>
std::optional<Enum> from_underlying(std::underlying_type_t<Enum> x) {
    using Traits = enum_traits<Enum>;

    if constexpr (is_enum_with_trivial_values<Enum>()) {
        return 0 <= x && std::cmp_less(x, Traits::elements.size()) ? std::make_optional(Enum(x))
                                                                   : std::nullopt;
    }

    auto it = std::find_if(Traits::elements.begin(), Traits::elements.end(), [x](Enum e) {
        return std::to_underlying(e) == x;
    });
    if (it != Traits::elements.end()) {
        return *it;
    }
    return std::nullopt;
}

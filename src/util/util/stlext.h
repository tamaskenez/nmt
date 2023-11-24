#pragma once

#include <string_view>
#include <variant>

// WARNING: error prone hacky macro, x must be a variable.
// If it's a function it will be invoked twice. Which is a performance issue for pure functions and
// incorrect behavior for functions with side effects.
#define BEGIN_END(x) std::begin(x), std::end(x)

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template<typename... Ts, typename Variant>
auto switch_variant(Variant&& variant, Ts&&... ts) {
    return std::visit(overloaded{std::forward<Ts>(ts)...}, std::forward<Variant>(variant));
}

std::string_view trim(std::string_view sv);  // Trim if `std::isspace()`.

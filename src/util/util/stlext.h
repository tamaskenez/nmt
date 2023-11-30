#pragma once

#include <cassert>
#include <filesystem>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
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

// Return path.u8string() but as a std::string (without any conversion).
std::string path_to_string(const std::filesystem::path& p);

// Return path.u8string() but as a std::string (without any conversion).
std::filesystem::path path_from_string(std::string_view sv);

template<class T>
auto data_plus_size(const T& container) {
    return container.data() + container.size();
}

// template<class T, class R >
// void append_range(std::vector<T>&v, R&&rg){
//	if constexpr (std::is_rvalue_reference_v<R>) {
//		v.insert(v.end(), std::make_move_iterator(std::ranges::begin(rg)),
// std::make_move_iterator(std::ranges::end(rg))); 	} else if constexpr
//(!std::is_rvalue_reference_v<R>){
//		//v.insert(v.end(), std::ranges::begin(rg), std::ranges::end(rg));
//	}
// }

template<class T, class R>
void append_range(std::vector<T>& v, R&& rg) {
    v.insert(v.end(),
             std::make_move_iterator(std::move(rg).begin()),
             std::make_move_iterator(std::move(rg).end()));
}

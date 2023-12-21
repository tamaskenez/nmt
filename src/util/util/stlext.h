#pragma once

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <version>

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

template<class T, class R>
void append_range(std::vector<T>& v, R&& rg) {
    v.insert(v.end(),
             std::make_move_iterator(std::move(rg).begin()),
             std::make_move_iterator(std::move(rg).end()));
}

template<class R, class T>
std::optional<R> try_convert_integer(T t) {
    if (std::in_range<R>(t)) {
        return R(t);
    }
    return std::nullopt;
}

inline bool shallow_equal(std::string_view x, std::string_view y) {
    return x.data() == y.data() && x.size() == y.size();
}

#if !defined __cpp_lib_ranges_contains || __cpp_lib_ranges_contains == 0
namespace std::ranges {
template<std::ranges::input_range R, class T, class Proj = std::identity>
requires std::indirect_binary_predicate<std::ranges::equal_to,
                                        std::projected<std::ranges::iterator_t<R>, Proj>,
                                        const T*>
constexpr bool contains(R&& r, const T& value, Proj proj = {}) {
    return std::ranges::find(r, value, proj) != std::ranges::end(r);
}
}  // namespace std::ranges
#endif

struct path_hash {
    size_t operator()(const std::filesystem::path& p) const {
        return std::filesystem::hash_value(p);
    }
};

template<class T>
void sort_unique_inplace(std::vector<T>& xs) {
    std::ranges::sort(xs);
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
}

template<class T>
auto make_vector(T&& single_item) {
    std::vector<std::decay_t<T>> v;
    v.push_back(std::move(single_item));
    return v;
}

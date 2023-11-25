#pragma once

#include <absl/log/log.h>

#include <concepts>
#include <expected>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>  // monostate

// std::expected helpers inspired by Swift's `try`, `try?`, `try!`
//
// For the explanation we'll be using the following sample functions:
//
//     std::expected<std::monostate, Error> void_or_error();
//     std::unexpected. std::expected<Something, Error> something_or_unexpexted();
//
// Use cases (expressed as Swift statements):
//
// 1. `try void_or_error()`
// 2. `let a = try something_or_unexpexted()`
// 3. `try! void_or_error()`
// 4. `let a = try! something_or_unexpexted()`
// 5. `try? void_or_error()`
// 6. `let a = try? something_or_unexpexted()`
//
// These uses cases can be expressed as follows:
//
// 1. `try void_or_error()`
//
//     TRY(void_or_error()); // if error, return std::unexpected(void_or_error().error());
//
// 2. `let a = try something_or_unexpexted()`
//
//     TRY_ASSIGN(x, something_or_unexpected());
//
// is equivalent to `return std::unexpected(something_or_unexpexted().error())` on error, or
// `auto x = *something_or_unexpexted(); // on success.
//
// 3. `try! void_or_error()`
// 4. `let a = try! something_or_unexpexted()`
//
//     TRY_OR_FATAL(void_or_error());
//
// equivalent to `LOG(FATAL) << void_or_error();` on error
//
//     auto a = TRY_OR_FATAL(something_or_unexpexted());
//
// on error, it's equivalent to
//
//     LOG(FATAL) << something_or_unexpexted().error();
//
// on success, it's equivalent to
//
//     auto x = *something_or_unexpexted();
//
// 5. `try? void_or_error()`
// 6. `let a = try? something_or_unexpexted()`
//
//     to_optional(void_or_error())
//     auto x = to_optional(something_or_unexpexted());

#define UTIL_PP_CONCAT2(A, B) A##B
#define UTIL_PP_CONCAT(A, B)  UTIL_PP_CONCAT2(A, B)

template<class T>
struct is_std_optional : std::false_type {};

template<class T>
struct is_std_optional<std::optional<T>> : std::true_type {};

template<class T>
inline constexpr bool is_std_optional_v = is_std_optional<T>::value;

template<class T>
struct is_std_expected : std::false_type {};

template<class T, class E>
struct is_std_expected<std::expected<T, E>> : std::true_type {};

template<class T>
inline constexpr bool is_std_expected_v = is_std_expected<T>::value;

#define _TRY_INTERNAL(EXPECTED, TMP_VAR, TMP_VAR_TYPE)                              \
    do {                                                                            \
        auto&& TMP_VAR = (EXPECTED);                                                \
        static_assert(std::is_rvalue_reference_v<decltype(TMP_VAR)>);               \
        using TMP_VAR_TYPE = std::decay_t<decltype(TMP_VAR)>;                       \
        /* assert that it's std::expected<std::monostate, E> */                     \
        static_assert(is_std_expected_v<TMP_VAR_TYPE>                               \
                      && std::is_same_v<TMP_VAR_TYPE::value_type, std::monostate>); \
        if (!TMP_VAR.has_value()) {                                                 \
            return std::unexpected(std::move(TMP_VAR.error()));                     \
        }                                                                           \
    } while (false)

#define TRY(EXPECTED)                                     \
    _TRY_INTERNAL(EXPECTED,                               \
                  UTIL_PP_CONCAT(_tmp_var_, __COUNTER__), \
                  UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__))

// Expands to sequence of statements, if mistakenly using as an if branch:
//
//     if(...) TRY_ASSIGN(a, b);
//
// will result in compiler error because the internal variable will be not in scope for the
// assignment.
#define _TRY_ASSIGN_INTERNAL(VAR, EXPECTED, TMP_VAR, TMP_VAR_TYPE) \
    auto&& TMP_VAR = (EXPECTED);                                   \
    static_assert(std::is_rvalue_reference_v<decltype(TMP_VAR)>);  \
    using TMP_VAR_TYPE = std::decay_t<decltype(TMP_VAR)>;          \
    /* assert that it's std::expected<T, E> */                     \
    static_assert(is_std_expected_v<TMP_VAR_TYPE>);                \
    if (!TMP_VAR.has_value()) {                                    \
        return std::unexpected(std::move(TMP_VAR.error()));        \
    }                                                              \
    auto&& VAR = std::move(*TMP_VAR)

#define TRY_ASSIGN(VAR, EXPECTED)                                \
    _TRY_ASSIGN_INTERNAL(VAR,                                    \
                         EXPECTED,                               \
                         UTIL_PP_CONCAT(_tmp_var_, __COUNTER__), \
                         UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__))

// __VA_ARGS__ are the arguments for std::unexpected()
#define _TRY_ASSIGN_OR_UNEXPECTED_INTERNAL(VAR, EXPECTED_OR_OPTIONAL, TMP_VAR, TMP_VAR_TYPE, ...) \
    auto&& TMP_VAR = (EXPECTED_OR_OPTIONAL);                                                      \
    static_assert(std::is_rvalue_reference_v<decltype(TMP_VAR)>);                                 \
    using TMP_VAR_TYPE = std::decay_t<decltype(TMP_VAR)>;                                         \
    /* assert that it's std::expected<T, E> or std::optional<T> */                                \
    static_assert(is_std_expected_v<TMP_VAR_TYPE> || is_std_optional_v<TMP_VAR_TYPE>);            \
    if (!TMP_VAR.has_value()) {                                                                   \
        return std::unexpected(__VA_ARGS__);                                                      \
    }                                                                                             \
    auto&& VAR = std::move(*TMP_VAR)

// __VA_ARGS__ are the arguments for std::unexpected()
#define TRY_ASSIGN_OR_UNEXPECTED(VAR, EXPECTED_OR_OPTIONAL, ...)                    \
    _TRY_ASSIGN_OR_UNEXPECTED_INTERNAL(VAR,                                         \
                                       EXPECTED_OR_OPTIONAL,                        \
                                       UTIL_PP_CONCAT(_tmp_var_, __COUNTER__),      \
                                       UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__), \
                                       __VA_ARGS__)

#ifndef _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL
#    define _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL(VAR)
#endif

namespace detail {
template<class T, class E>
void or_fatal(std::expected<T, E>&& e) {
    const bool panic = !e.has_value();
    _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL(if (panic) { throw util_test_error(e.error()); })
    LOG_IF(FATAL, panic) << e.error();
}
}  // namespace detail

// Not a macro, still full-caps to stand out like the rest.
template<class E>
void TRY_OR_FATAL(std::expected<std::monostate, E>&& e) {
    detail::or_fatal(std::move(e));
}

template<class T, class E>
void TRY_OR_FATAL(std::expected<T, E>&& e)
requires std::same_as<T, std::monostate>
{
    const bool panic = !e.has_value();
    _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL(if (panic) { throw util_test_error(e.error()); })
    LOG_IF(FATAL, panic) << e.error();
}

template<class T, class E>
[[nodiscard]] T TRY_OR_FATAL(std::expected<T, E>&& e)
requires(!std::same_as<T, std::monostate>)
{
    const bool panic = !e.has_value();
    _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL(if (panic) { throw util_test_error(e.error()); })
    LOG_IF(FATAL, panic) << e.error();
    return std::move(*e);
}

template<class T, class E>
std::optional<T> to_optional(std::expected<T, E>&& e) {
    if (e) {
        return std::move(*e);
    }
    return std::nullopt;
}

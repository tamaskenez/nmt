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

// Continue if no error, return unexpected otherwise:
//
//     TRY(<std::expected-rvalue>):
//
// Example usage:
//
//     TRY(foo()); // foo returns std::expected<std::monostate, E>
//
// Equivalent to
//
// auto tmp = foo();
// if (!tmp) { return std::unexpected(std::move(tmp.error())); }
//
#define TRY(EXPECTED)                                     \
    _TRY_INTERNAL(EXPECTED,                               \
                  UTIL_PP_CONCAT(_tmp_var_, __COUNTER__), \
                  UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__))

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

// Assign if no error, return unexpected otherwise:
//
//     TRY_ASSIGN(<variable-name>, <std::expected-rvalue>):
//
// Example usage:
//
//     TRY_ASSIGN(var, foo()); // foo returns std::expected<T, E>
//
// Equivalent to
//
// auto tmp = foo();
// if (!tmp) { return std::unexpected(std::move(tmp.error())); }
// auto&& var = std::move(*tmp);
//
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

namespace detail {
template<class T>
struct ErrorOrNullopt;

template<class T>
struct ErrorOrNullopt<std::optional<T>> {
    std::nullopt_t operator()(const std::optional<T>&) const {
        return std::nullopt;
    }
};

template<class T, class E>
struct ErrorOrNullopt<std::expected<T, E>> {
    E& operator()(std::expected<T, E>& e) const {
        return e.error();
    }
};
}  // namespace detail

// Assign if no error, return unexpected otherwise:
//
//     TRY_ASSIGN_OR_UNEXPECTED(<var>, <std::expected-rvalue, args, ...):
//     TRY_ASSIGN_OR_UNEXPECTED(<var>, <std::optional-rvalue, args, ...):
//
// Example usage:
//
//     TRY_ASSIGN(var, foo(), "failed");
//
// Equivalent to
//
// auto tmp = foo();
// if (!tmp) { return std::unexpected(args...)); }
// auto&& var = std::move(*tmp);
//
// __VA_ARGS__ are the arguments for std::unexpected()
#define TRY_ASSIGN_OR_UNEXPECTED(VAR, EXPECTED_OR_OPTIONAL, ...)                    \
    _TRY_ASSIGN_OR_UNEXPECTED_INTERNAL(VAR,                                         \
                                       EXPECTED_OR_OPTIONAL,                        \
                                       UTIL_PP_CONCAT(_tmp_var_, __COUNTER__),      \
                                       UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__), \
                                       __VA_ARGS__)

// __VA_ARGS__ are the arguments for std::unexpected()
#define _TRY_ASSIGN_OR_RETURN_VALUE_INTERNAL(                                          \
    VAR, EXPECTED_OR_OPTIONAL, TMP_VAR, TMP_VAR_TYPE, VALUE)                           \
    auto&& TMP_VAR = (EXPECTED_OR_OPTIONAL);                                           \
    static_assert(std::is_rvalue_reference_v<decltype(TMP_VAR)>);                      \
    using TMP_VAR_TYPE = std::decay_t<decltype(TMP_VAR)>;                              \
    /* assert that it's std::expected<T, E> or std::optional<T> */                     \
    static_assert(is_std_expected_v<TMP_VAR_TYPE> || is_std_optional_v<TMP_VAR_TYPE>); \
    if (!TMP_VAR.has_value()) {                                                        \
        [[maybe_unused]] auto&& UNEXPECTED_ERROR =                                     \
            detail::ErrorOrNullopt<TMP_VAR_TYPE>()(TMP_VAR);                           \
        return VALUE;                                                                  \
    }                                                                                  \
    auto&& VAR = std::move(*TMP_VAR)

// Assign if no error, return unexpected otherwise:
//
//     TRY_ASSIGN_OR_RETURN_VALUE(<var>, <std::expected-rvalue, args, value):
//     TRY_ASSIGN_OR_RETURN_VALUE(<var>, <std::optional-rvalue, args, value):
//
// The `value` can be a complex expression and it can use the variable UNEXPECTED_ERROR which is a
// reference to the `std::expected<T,E>::error()` or std::nullopt.
//
// Example usage:
//
//     TRY_ASSIGN(var, foo(), "failed");
//     TRY_ASSIGN(var, foo(), UNEXPECTED_ERROR == 3 ? "OK" : "NOTOK");
//
// Equivalent to
//
// auto tmp = foo();
// if (!tmp) {
//     auto& UNEXPECTED_ERROR = tmp.error();
//     return value;
// }
// auto&& var = std::move(*tmp);
//
// __VA_ARGS__ are the arguments for std::unexpected()
#define TRY_ASSIGN_OR_RETURN_VALUE(VAR, EXPECTED_OR_OPTIONAL, VALUE)                  \
    _TRY_ASSIGN_OR_RETURN_VALUE_INTERNAL(VAR,                                         \
                                         EXPECTED_OR_OPTIONAL,                        \
                                         UTIL_PP_CONCAT(_tmp_var_, __COUNTER__),      \
                                         UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__), \
                                         VALUE)

#ifndef _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL
#    define _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL(VAR)
#endif

#if 0
namespace detail {
template<class T, class E>
void or_fatal(std::expected<T, E>&& e) {
    const bool panic = !e.has_value();
    _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL(if (panic) { throw util_test_error(e.error()); })
    LOG_IF(FATAL, panic) << e.error();
}
}  // namespace detail

template<class E>
void TRY_OR_FATAL(std::expected<std::monostate, E>&& e) {
    detail::or_fatal(std::move(e));
}
#endif

// Not a macro, still full-caps to stand out like the rest.
// It's possible to implement these without a macro because we don't need to return.
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

template<class T>
[[nodiscard]] T TRY_OR_FATAL(std::optional<T>&& e) {
    const bool panic = !e.has_value();
    _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL(if (panic) { throw util_test_error("<nullopt>"); })
    LOG_IF(FATAL, panic) << "Accessing value of <nullopt>.";
    return std::move(*e);
}

template<class T, class E>
std::optional<T> to_optional(std::expected<T, E>&& e) {
    if (e) {
        return std::move(*e);
    }
    return std::nullopt;
}

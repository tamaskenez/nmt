#pragma once

#include <absl/log/log.h>

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
//     OR_RETHROW(void_or_error()); // if error, return std::unexpected(void_or_error().error());
//
// 2. `let a = try something_or_unexpexted()`
//
//     ASSIGN_OR_RETHROW(x, something_or_unexpexted());
//
// is equivalent to `return std::unexpected(something_or_unexpexted().error())` on error, or
// `auto x = *something_or_unexpexted(); // on success.
//
// 3. `try! void_or_error()`
//
//     OR_FATAL(void_or_error()); // equivalent to `LOG(FATAL) << void_or_error();` on error
//
// 4. `let a = try! something_or_unexpexted()`
//
//     ASSIGN_OR_FATAL(x, something_or_unexpexted());
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
//     auto x= to_optional(something_or_unexpexted());

#define UTIL_PP_CONCAT2(A, B) A##B
#define UTIL_PP_CONCAT(A, B)  UTIL_PP_CONCAT2(A, B)

#define OR_RETHROW2(EXPECTED, TMP_VAR, TMP_VAR_TYPE)                                            \
    do {                                                                                        \
        auto&& TMP_VAR = (EXPECTED);                                                            \
        static_assert(std::is_rvalue_reference_v<decltype(TMP_VAR)>);                           \
        using TMP_VAR_TYPE = std::decay_t<decltype(TMP_VAR)>;                                   \
        /* assert that it's std::expected<std::monostate, E> */                                 \
        static_assert(std::is_same_v<TMP_VAR_TYPE,                                              \
                                     std::expected<std::monostate, TMP_VAR_TYPE::error_type>>); \
        if (!TMP_VAR.has_value()) {                                                             \
            return std::unexpected(std::move(TMP_VAR.error()));                                 \
        }                                                                                       \
    } while (false)

#define OR_RETHROW(EXPECTED)                            \
    OR_RETHROW2(EXPECTED,                               \
                UTIL_PP_CONCAT(_tmp_var_, __COUNTER__), \
                UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__))

// Expands to sequence of statements, if mistakenly using as an if branch:
//
//     if(...) ASSIGN_OR_RETHROW(a, b);
//
// will result in compiler error because the internal variable will be not in scope for the
// assignment.
#define ASSIGN_OR_RETHROW2(X, EXPECTED, TMP_VAR, TMP_VAR_TYPE)                              \
    auto&& TMP_VAR = (EXPECTED);                                                            \
    static_assert(std::is_rvalue_reference_v<decltype(TMP_VAR)>);                           \
    using TMP_VAR_TYPE = std::decay_t<decltype(TMP_VAR)>;                                   \
    /* assert that it's std::expected<T, E> */                                              \
    static_assert(                                                                          \
        std::is_same_v<TMP_VAR_TYPE,                                                        \
                       std::expected<TMP_VAR_TYPE::value_type, TMP_VAR_TYPE::error_type>>); \
    if (!TMP_VAR.has_value()) {                                                             \
        return std::unexpected(std::move(TMP_VAR.error()));                                 \
    }                                                                                       \
    auto&& X = std::move(*TMP_VAR);

#define ASSIGN_OR_RETHROW(X, EXPECTED)                         \
    ASSIGN_OR_RETHROW2(X,                                      \
                       EXPECTED,                               \
                       UTIL_PP_CONCAT(_tmp_var_, __COUNTER__), \
                       UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__))

#ifndef UTIL_ERROR_ENABLE_IF_TESTING
#    define UTIL_ERROR_ENABLE_IF_TESTING(X)
#endif

namespace detail {
template<class T, class E>
void or_fatal(std::expected<T, E>&& e) {
    const bool panic = !e.has_value();
    UTIL_ERROR_ENABLE_IF_TESTING(if (panic) { throw util_test_error(e.error()); })
    LOG_IF(FATAL, panic) << e.error();
}
}  // namespace detail

// Not a macro, still full-caps to stand out like the rest.
template<class E>
void OR_FATAL(std::expected<std::monostate, E>&& e) {
    detail::or_fatal(std::move(e));
}

// Expands to sequence of statements, if mistakenly using as an if branch:
//
//     if(...) ASSIGN_OR_RETHROW(a, b);
//
// will result in compiler error because the internal variable will be not in scope for the
// assignment.
#define ASSIGN_OR_FATAL2(X, EXPECTED, TMP_VAR, TMP_VAR_TYPE)                                \
    auto&& TMP_VAR = (EXPECTED);                                                            \
    static_assert(std::is_rvalue_reference_v<decltype(TMP_VAR)>);                           \
    using TMP_VAR_TYPE = std::decay_t<decltype(TMP_VAR)>;                                   \
    /* assert that it's std::expected<T, E> */                                              \
    static_assert(                                                                          \
        std::is_same_v<TMP_VAR_TYPE,                                                        \
                       std::expected<TMP_VAR_TYPE::value_type, TMP_VAR_TYPE::error_type>>); \
    detail::or_fatal(std::move(TMP_VAR));                                                   \
    auto&& X = std::move(*TMP_VAR);

#define ASSIGN_OR_FATAL(X, EXPECTED)                         \
    ASSIGN_OR_FATAL2(X,                                      \
                     EXPECTED,                               \
                     UTIL_PP_CONCAT(_tmp_var_, __COUNTER__), \
                     UTIL_PP_CONCAT(_tmp_var_type_, __COUNTER__))

template<class T, class E>
std::optional<T> to_optional(std::expected<T, E>&& e) {
    if (e) {
        return std::move(*e);
    }
    return std::nullopt;
}

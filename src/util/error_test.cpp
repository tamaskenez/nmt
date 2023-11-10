#include <stdexcept>
#include <string>
#include <utility>

#define _UTIL_ERROR_ENABLE_IF_TESTING_INTERNAL(X) X

class util_test_error : public std::logic_error {
   public:
    util_test_error(std::string s)
        : std::logic_error(std::move(s)) {}
};

#include <util/error.h>

#include <memory>

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

namespace {
using value_type = std::unique_ptr<int>;
using EXV = std::expected<std::monostate, std::string>;
using EXI = std::expected<value_type, std::string>;

constexpr std::string_view k_errorString = "some error";
constexpr int k_okInt = 345;

enum class StatementReached { yes, no };
std::optional<StatementReached> statementReached;

EXV exv_or_rethrow(bool ok) {
    statementReached = StatementReached::no;
    TRY(ok ? EXV() : EXV(std::unexpected(k_errorString)));
    statementReached = StatementReached::yes;
    return {};
}

EXI exi_or_rethrow(bool ok) {
    statementReached = StatementReached::no;
    auto value = std::make_unique<int>(k_okInt);
    TRY_ASSIGN(x, ok ? EXI(std::in_place, std::move(value)) : EXI(std::unexpected(k_errorString)));
    static_assert(std::is_same_v<std::decay_t<decltype(x)>, value_type>);
    statementReached = StatementReached::yes;
    return x;
}

value_type exi_assign_or_fatal(bool ok) {
    statementReached = StatementReached::no;
    auto value = std::make_unique<int>(k_okInt);
    auto x = TRY_OR_FATAL(ok ? EXI(std::in_place, std::move(value))
                             : EXI(std::unexpected(k_errorString)));
    static_assert(std::is_same_v<std::decay_t<decltype(x)>, value_type>);
    statementReached = StatementReached::yes;
    return x;
}

}  // namespace

TEST(error, TRY_ok) {
    statementReached.reset();
    ASSERT_TRUE(exv_or_rethrow(true).has_value());
    ASSERT_EQ(statementReached, StatementReached::yes);
}

TEST(error, TRY_error) {
    statementReached.reset();
    auto r = exv_or_rethrow(false);
    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(r.error(), k_errorString);
    ASSERT_EQ(statementReached, StatementReached::no);
}

TEST(error, TRY_ASSIGN_ok) {
    statementReached.reset();
    auto r = exi_or_rethrow(true);
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(**r, k_okInt);
    ASSERT_EQ(statementReached, StatementReached::yes);
}

TEST(error, TRY_ASSIGN_error) {
    statementReached.reset();
    auto r = exi_or_rethrow(false);
    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(r.error(), k_errorString);
    ASSERT_EQ(statementReached, StatementReached::no);
}

void or_fatal_error_test() {
    TRY_OR_FATAL(EXV(std::unexpected(k_errorString)));
}

TEST(error, void_TRY_OR_FATAL_ok) {
    TRY_OR_FATAL(EXV());
    SUCCEED();
}

TEST(error, void_TRY_OR_FATAL_error) {
    try {
        or_fatal_error_test();
        FAIL();
    } catch (const util_test_error& e) {
        ASSERT_EQ(e.what(), k_errorString);
    }
}

TEST(error, nonvoid_TRY_OR_FATAL_ok) {
    statementReached.reset();
    auto r = exi_assign_or_fatal(true);
    ASSERT_EQ(*r, k_okInt);
    ASSERT_EQ(statementReached, StatementReached::yes);
}

TEST(error, nonvoid_TRY_OR_FATAL_error) {
    try {
        statementReached.reset();
        exi_assign_or_fatal(false);
        FAIL();
    } catch (const util_test_error& e) {
        ASSERT_EQ(statementReached, StatementReached::no);
        ASSERT_EQ(e.what(), k_errorString);
    }
}

TEST(error, to_optional_ok) {
    value_type value = std::make_unique<int>(k_okInt);
    auto x = to_optional(EXI(std::in_place, std::move(value)));
    static_assert(std::is_same_v<decltype(x), std::optional<value_type>>);
    ASSERT_TRUE(x);
    ASSERT_EQ(**x, k_okInt);
}

TEST(error, to_optional_error) {
    auto x = to_optional(EXI(std::unexpected(k_errorString)));
    static_assert(std::is_same_v<decltype(x), std::optional<value_type>>);
    ASSERT_FALSE(x);
}

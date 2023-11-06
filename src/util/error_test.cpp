#include <stdexcept>
#include <string>
#include <utility>

#define UTIL_ERROR_ENABLE_IF_TESTING(X) X

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
    OR_RETHROW(ok ? EXV() : EXV(std::unexpected(k_errorString)));
    statementReached = StatementReached::yes;
    return {};
}

EXI exi_or_rethrow(bool ok) {
    statementReached = StatementReached::no;
    auto value = std::make_unique<int>(k_okInt);
    ASSIGN_OR_RETHROW(
        x, ok ? EXI(std::in_place, std::move(value)) : EXI(std::unexpected(k_errorString)));
    static_assert(std::is_same_v<std::decay_t<decltype(x)>, value_type>);
    statementReached = StatementReached::yes;
    return x;
}

value_type exi_assign_or_fatal(bool ok) {
    statementReached = StatementReached::no;
    auto value = std::make_unique<int>(k_okInt);
    ASSIGN_OR_FATAL(
        x, ok ? EXI(std::in_place, std::move(value)) : EXI(std::unexpected(k_errorString)));
    static_assert(std::is_same_v<std::decay_t<decltype(x)>, value_type>);
    statementReached = StatementReached::yes;
    return x;
}

}  // namespace

TEST(error, OR_RETHROW_ok) {
    statementReached.reset();
    ASSERT_TRUE(exv_or_rethrow(true).has_value());
    ASSERT_EQ(statementReached, StatementReached::yes);
}

TEST(error, OR_RETHROW_error) {
    statementReached.reset();
    auto r = exv_or_rethrow(false);
    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(r.error(), k_errorString);
    ASSERT_EQ(statementReached, StatementReached::no);
}

TEST(error, ASSIGN_OR_RETHROW_ok) {
    statementReached.reset();
    auto r = exi_or_rethrow(true);
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(**r, k_okInt);
    ASSERT_EQ(statementReached, StatementReached::yes);
}

TEST(error, ASSIGN_OR_RETHROW_error) {
    statementReached.reset();
    auto r = exi_or_rethrow(false);
    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(r.error(), k_errorString);
    ASSERT_EQ(statementReached, StatementReached::no);
}

void or_fatal_error_test() {
    OR_FATAL(EXV(std::unexpected(k_errorString)));
}

TEST(error, OR_FATAL_ok) {
    OR_FATAL(EXV());
    SUCCEED();
}

TEST(error, OR_FATAL_error) {
    try {
        or_fatal_error_test();
        FAIL();
    } catch (const util_test_error& e) {
        ASSERT_EQ(e.what(), k_errorString);
    }
}

TEST(error, ASSIGN_OR_FATAL_ok) {
    statementReached.reset();
    auto r = exi_assign_or_fatal(true);
    ASSERT_EQ(*r, k_okInt);
    ASSERT_EQ(statementReached, StatementReached::yes);
}

TEST(error, ASSIGN_OR_FATAL_error) {
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

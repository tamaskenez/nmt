#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace libtokenizer {
enum class TokenType { tok, kw, num, id, hash };
struct Token {
    TokenType type;
    std::string value;
    std::string_view sourceValue;
    bool IsInlineComment() const;
};

struct Result {
    std::vector<Token> tokens;
};
std::expected<Result, std::string> process_cpp_with_option_B(std::string_view in_sv);
}  // namespace libtokenizer

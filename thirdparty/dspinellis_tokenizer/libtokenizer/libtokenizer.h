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

    bool IsInlineComment() const {
        return type == TokenType::tok && value == "//...";
    }
    bool IsBlockComment() const {
        return type == TokenType::tok && value == "/*...*/";
    }
    bool IsComment() const {
        return IsInlineComment() || IsBlockComment();
    }
};

std::string_view to_string_view(TokenType t);

struct Result {
    std::vector<Token> tokens;
};

std::expected<Result, std::string> process_cpp_with_option_B(std::string_view in_sv);

}  // namespace libtokenizer

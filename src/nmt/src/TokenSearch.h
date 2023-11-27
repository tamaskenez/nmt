#pragma once

struct TokenSearchResult {
    std::optional<std::string> error;
};

class TokenSearch {
   public:
    using Token = libtokenizer::Token;
    using TokenType = libtokenizer::TokenType;
    explicit TokenSearch(std::span<const Token> tokens)
        : tokens(tokens)
        , nextIdxOrError(0) {}
    // Skip comments and eat next token which must the specified one, then advance to next.
    TokenSearch& Eat(TokenType tokenType, std::string_view text) {
        if (!nextIdxOrError) {
            return *this;
        }
        auto& nextIdx = *nextIdxOrError;
        bool found = false;
        SkipComments();
        if (nextIdx < tokens.size()) {
            auto& t = tokens[nextIdx];
            if (t.type == tokenType && t.sourceValue == text) {
                found = true;
            }
        }
        if (found) {
            ++nextIdx;
        } else {
            FailWith(fmt::format(
                "Can't find token {} with text {}", libtokenizer::to_string_view(tokenType), text));
        }
        return *this;
    }
    // Find next specified token then advance to the next after that.
    TokenSearch& Find(TokenType tokenType, std::string_view text, size_t* idx = nullptr) {
        if (!nextIdxOrError) {
            return *this;
        }
        auto& nextIdx = *nextIdxOrError;
        bool found = false;
        for (;;) {
            if (nextIdx < tokens.size()) {
                auto& t = tokens[nextIdx];
                if (t.type == tokenType && t.sourceValue == text) {
                    found = true;
                    if (idx) {
                        *idx = nextIdx;
                    }
                } else {
                    ++nextIdx;
                    continue;
                }
            } else {
                break;
            }
        }
        if (found) {
            ++nextIdx;
        } else {
            FailWith(fmt::format(
                "Can't find token {} with text {}", libtokenizer::to_string_view(tokenType), text));
        }
        return *this;
    }
    // Assert that the current token is that one that is asked for, don't advance pointer.
    TokenSearch& Assert(TokenType tokenType, std::string_view text) {
        if (!nextIdxOrError) {
            return *this;
        }
        SkipComments();
        auto& nextIdx = *nextIdxOrError;
        bool found = false;
        if (nextIdx >= tokens.size()) {
            auto& t = tokens[nextIdx];
            found = t.type == tokenType && t.sourceValue == text;
        }
        if (!found) {
            FailWith(fmt::format(
                "Can't find token {} with text {}", libtokenizer::to_string_view(tokenType), text));
        }
        return *this;
    }
    // Go to the last, non-comment token. It's an error if there isn't one.
    TokenSearch& GoToLast() {
        if (!nextIdxOrError || tokens.empty()) {
            return *this;
        }
        auto& nextIdx = *nextIdxOrError;
        nextIdx = tokens.size() - 1;
        while (nextIdx > 0 && tokens[nextIdx].IsComment()) {
            --nextIdx;
        }
        if (tokens[nextIdx].IsComment()) {
            FailWith("No last token.");
        }
        return *this;
    }
    // Go to the previous, non-comment token.
    TokenSearch& GoToPrevious() {
        if (!nextIdxOrError || tokens.empty()) {
            return *this;
        }
        auto& nextIdx = *nextIdxOrError;
        if (nextIdx == 0) {
            FailWith("No previous token");
        } else {
            --nextIdx;
            while (nextIdx > 0 && tokens[nextIdx].IsComment()) {
                --nextIdx;
            }
            if (tokens[nextIdx].IsComment()) {
                FailWith("No previous token");
            }
        }
        return *this;
    }
    bool Failed() const {
        return nextIdxOrError.has_value();
    }
    void SkipComments() {
        if (!nextIdxOrError) {
            return;
        }
        auto& nextIdx = *nextIdxOrError;
        while (nextIdx < tokens.size() && tokens[nextIdx].IsComment()) {
            ++nextIdx;
        }
    }
    void FailWith(std::string error) {
        nextIdxOrError = std::unexpected(std::move(error));
    }
    std::expected<std::monostate, std::string> FinishSearch() && {
        if (nextIdxOrError) {
            return std::monostate();
        }
        return std::unexpected(std::move(nextIdxOrError.error()));
    }

   private:
    std::span<const Token> tokens;
    std::expected<size_t, std::string> nextIdxOrError;
};

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
        , nextIdxOrError(k_justStartedIdx) {}
    // Skip comments and eat next token which must the specified one, then advance to next.
    TokenSearch& Eat(TokenType tokenType, std::string_view text, size_t* idx = nullptr) {
        if (!nextIdxOrError) {
            return *this;
        }
        if (auto e = AdvanceToNext(); !e) {
            FailWith(e.error());
            return *this;
        }
        auto& nextIdx = *nextIdxOrError;
        assert(nextIdx < tokens.size());
        auto& t = tokens[nextIdx];
        bool found = false;
        if (t.type == tokenType && t.sourceValue == text) {
            found = true;
            if (idx) {
                *idx = nextIdx;
            }
        }
        if (!found) {
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
        bool found = false;
        for (;;) {
            if (auto e = AdvanceToNext(); !e) {
                FailWith(e.error());
                return *this;
            }
            auto& nextIdx = *nextIdxOrError;
            assert(nextIdx < tokens.size());
            auto& t = tokens[nextIdx];
            if (t.type == tokenType && t.sourceValue == text) {
                found = true;
                if (idx) {
                    *idx = nextIdx;
                }
                break;
            }
        }
        if (!found) {
            FailWith(fmt::format(
                "Can't find token {} with text {}", libtokenizer::to_string_view(tokenType), text));
        }
        return *this;
    }
    TokenSearch& AssertEnd() {
        if (!nextIdxOrError) {
            return *this;
        }
        auto& nextIdx = *nextIdxOrError;
        bool ok = tokens.empty() && nextIdx == k_justStartedIdx;
        if (!ok) {
            if (nextIdx == k_justStartedIdx) {
                nextIdx = 0;
            }
            while (nextIdx + 1 < tokens.size() && tokens[nextIdx + 1].IsComment()) {
                ++nextIdx;
            }
            ok = nextIdx + 1 == tokens.size();
        }
        if (!ok) {
            FailWith("Didn't reach end");
        }
        return *this;
    }
    bool Failed() const {
        return nextIdxOrError.has_value();
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
    static constexpr size_t k_justStartedIdx = SIZE_T_MAX;
    static constexpr std::string_view k_openingBrackets = "([{";
    static constexpr std::string_view k_closingBrackets = ")]}";
    static std::optional<size_t> FindOpeningBracketIdx(char c) {
        auto idx = k_openingBrackets.find(c);
        return idx == std::string_view::npos ? std::nullopt : std::make_optional(idx);
    }
    static std::optional<size_t> FindClosingBracketIdx(char c) {
        auto idx = k_closingBrackets.find(c);
        return idx == std::string_view::npos ? std::nullopt : std::make_optional(idx);
    }

    std::span<const Token> tokens;
    // nextIdx = nextIdxOrError.value() points to the last found token.
    // Before the first search its value is k_justStartedIdx
    std::expected<size_t, std::string> nextIdxOrError;

    std::expected<std::monostate, std::string> AdvanceToNext() {
        assert(nextIdxOrError);
        auto& nextIdx = *nextIdxOrError;
        if (nextIdx == k_justStartedIdx) {
            if (tokens.empty()) {
                return std::unexpected("Tokens are empty");
            }
            nextIdx = 0;
            return {};
        }
        for (;;) {
            assert(nextIdx < tokens.size());
            auto& t = tokens[nextIdx];
            // Check for opening bracket, might need to skip until closing.
            if (t.IsSingleCharToken()) {
                if (auto bix = FindOpeningBracketIdx(t.sourceValue[0])) {
                    // Skip everything until the pair of this.
                    return CloseBracketInternal(k_closingBrackets[*bix]);
                }
            }
            if (nextIdx + 1 >= tokens.size()) {
                return std::unexpected("End of tokens reached.");
            }
            // Check for closing bracket, if so, we can't advance.
            auto& tn = tokens[nextIdx + 1];
            if (tn.IsSingleCharToken() && FindClosingBracketIdx(tn.sourceValue[0])) {
                return std::unexpected("Closing bracket reached.");
            }
            ++nextIdx;
            if (!tn.IsComment()) {
                return {};
            }
        }
    }

    // We're past an opening bracket `(`, `[` or `{`. Find the closing one, keeping track of the
    // other ones, too.
    std::expected<std::monostate, std::string> CloseBracketInternal(char closingBracket,
                                                                    size_t* foundIdx = nullptr) {
        CHECK(nextIdxOrError);
        auto& nextIdx = *nextIdxOrError;
        std::vector<char> stack = {closingBracket};
        CHECK(FindClosingBracketIdx(closingBracket));

        CHECK(!tokens.empty()
              && nextIdx < tokens.size());  // We had an opening bracket, tokens can't be empty.
        for (;;) {
            if (nextIdx + 1 >= tokens.size()) {
                return std::unexpected(
                    fmt::format("End of tokens reached while looking for `{}`", closingBracket));
            }
            ++nextIdx;
            auto& t = tokens[nextIdx];
            if (t.IsComment() || !t.IsSingleCharToken()) {
                continue;
            }
            auto c = t.value[0];

            if (auto idx = FindOpeningBracketIdx(c)) {
                stack.push_back(k_closingBrackets[*idx]);
            } else if ((idx = FindClosingBracketIdx(c))) {
                assert(!stack.empty());
                if (stack.back() != c) {
                    return std::unexpected(
                        fmt::format("Mismatched brackets, expected: {}, got {}", stack.back(), c));
                }
                stack.pop_back();
                if (stack.empty()) {
                    if (foundIdx) {
                        *foundIdx = nextIdx;
                    }
                    break;
                }
            }
        }
        return {};
    }
};

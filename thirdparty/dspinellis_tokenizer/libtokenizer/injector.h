#pragma once

#ifdef LIBTOKENIZER
#    include "libtokenizer.h"

#    include <cassert>
#    include <functional>
#    include <string>
#    include <utility>

#endif

#include <iostream>

#define injector_assert(COND) \
    injector::throw_tokenizer_error_if(COND, __FILE__, __LINE__, __func__, #COND)

#ifdef LIBTOKENIZER
#    define main(ARGC, ARGV) ignore_main(ARGC, ARGV)
#endif

namespace injector {
class tokenizer_error : public std::runtime_error {
   public:
    explicit tokenizer_error(const std::string& what_arg)
        : std::runtime_error(what_arg) {}
    explicit tokenizer_error(const char* what_arg)
        : std::runtime_error(what_arg) {}
};
inline void throw_tokenizer_error_if(
    bool cond, const char* file, int line, const char* func, const char* cond_string) {
    if (!cond) {
        throw tokenizer_error(std::string("Assertion failed: (") + cond_string + "), function "
                              + func + ", file " + file + ", line " + std::to_string(line));
    }
}

#ifdef LIBTOKENIZER

// Class with operator<< which the original tokenizer code can use instead of std::cout.
class Cout {
   public:
    using Result = libtokenizer::Result;
    void StartCapture(std::string_view source,
                      std::function<int()> currentSourceCharIdxFn,
                      Result* result) {
        capture = Capture{.source = source,
                          .currentSourceCharIdxFn = std::move(currentSourceCharIdxFn),
                          .result = result};
    }
    template<class T>
    Cout& operator<<(T&& value) {
        if (!capture) {
            std::cout << std::forward<T>(value);
            return *this;
        }
        if (!capture->tokenTypeOfLine) {
            // We're at the beginning of the line, expecting token type.
            injector_assert(!capture->tokenValueOfLine);
            if constexpr (!std::is_convertible_v<T, std::string_view>) {
                injector_assert(false);
            } else {
                SetTokenAtStartOfLine(std::string_view(value));
            }
        } else if (!capture->tokenValueOfLine) {
            // We're after the token type, expecting token value.
            if constexpr (std::is_convertible_v<T, std::string>) {
                capture->tokenValueOfLine = value;
            } else if constexpr (std::is_same_v<std::decay_t<T>, char>) {
                capture->tokenValueOfLine.emplace(1, value);
            } else {
                capture->tokenValueOfLine = std::to_string(value);
            }
        } else {
            // We're after the token type and value, expecting newline.
            if constexpr (std::is_same_v<std::decay_t<T>, char>) {
                injector_assert(value == '\n');
                EndLine();
            } else {
                injector_assert(false);
            }
        }
        return *this;
    }

   private:
    using TokenType = libtokenizer::TokenType;
    using Token = libtokenizer::Token;
    struct Capture {
        std::string_view source;
        std::function<int()> currentSourceCharIdxFn;
        Result* result{};
        std::optional<TokenType> tokenTypeOfLine;
        std::optional<std::string> tokenValueOfLine;
        int previousSourceCharIdx = 0;
    };
    std::optional<Capture> capture;

    void SetTokenAtStartOfLine(std::string_view t);
    void EndLine();
};
#endif

// TODO tokenizer writes to std::cerr, too.

#ifdef LIBTOKENIZER
thread_local extern Cout cout;
#else
extern std::ostream& cout = std::cout;
#endif
}  // namespace injector

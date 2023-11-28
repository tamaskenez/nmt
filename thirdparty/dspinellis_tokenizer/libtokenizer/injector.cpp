#include "injector.h"

#include <algorithm>

namespace injector {

#ifdef LIBTOKENIZER
thread_local Cout cout;
#else
std::ostream& cout = std::cout;
#endif

namespace {
std::string_view TrimRight(std::string_view sv) {
    while (!sv.empty() && std::isspace(sv.back())) {
        sv.remove_suffix(1);
    }
    return sv;
}

std::string_view Trim(std::string_view sv) {
    sv = TrimRight(sv);
    while (!sv.empty() && std::isspace(sv.front())) {
        sv.remove_prefix(1);
    }
    return sv;
}
}  // namespace
void Cout::SetTokenAtStartOfLine(std::string_view t) {
    t = TrimRight(t);
    if (t == "TOK") {
        capture->tokenTypeOfLine = TokenType::tok;
    } else if (t == "KW") {
        capture->tokenTypeOfLine = TokenType::kw;
    } else if (t == "NUM") {
        capture->tokenTypeOfLine = TokenType::num;
    } else if (t == "NUM 0") {
        capture->tokenTypeOfLine = TokenType::num;
        capture->tokenValueOfLine = "0";
    } else if (t == "ID") {
        capture->tokenTypeOfLine = TokenType::id;
    } else if (t == "HASH") {
        capture->tokenTypeOfLine = TokenType::hash;
    } else {
        injector_assert(false);
    }
}

void Cout::EndLine() {
    const int startIdx = capture->previousSourceCharIdx;
    const int endIdx = capture->currentSourceCharIdxFn();
    const auto from = std::clamp(startIdx, 0, int(capture->source.size()));
    const auto to = std::clamp(endIdx, 0, int(capture->source.size()));
    auto sourceValue = Trim(capture->source.substr(from, to - from));
    capture->result->tokens.push_back(Token{.type = *capture->tokenTypeOfLine,
                                            .value = std::move(*capture->tokenValueOfLine),
                                            .sourceValue = sourceValue});
    capture->tokenTypeOfLine.reset();
    capture->tokenValueOfLine.reset();
    capture->previousSourceCharIdx = endIdx;
}
}  // namespace injector

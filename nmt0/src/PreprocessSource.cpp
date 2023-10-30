#include "pch.h"

#include "PreprocessSource.h"
#include "TryEatSpecialComment.h"
#include "parse.h"

std::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv) {
    enum class State { atStartOfLine, inMiddleOfLine } state = State::atStartOfLine;
    std::vector<SpecialComment> specialComments;
    std::vector<std::string_view> nonCommentCode;
    std::optional<const char*> currentNonComment;
    auto closeCurrentNonComment = [&currentNonComment, &sv, &nonCommentCode]() {
        if (currentNonComment) {
            auto size = sv.data() - *currentNonComment;
            CHECK(size >= 0);
            nonCommentCode.push_back(std::string_view(*currentNonComment, size_t(size)));
            currentNonComment.reset();
        }
    };
    auto makeCurrentNonComment = [&currentNonComment, &sv]() {
        if (!currentNonComment) {
            currentNonComment = sv.data();
        }
    };
    while (!sv.empty()) {
        switch (state) {
            case State::atStartOfLine: {
                sv = EatBlank(sv);
                auto specialCommentAndRestOr = TryEatSpecialComment(sv);
                if (specialCommentAndRestOr) {
                    // Valid special comment.
                    closeCurrentNonComment();
                    specialComments.push_back(std::move(specialCommentAndRestOr->first));
                    sv = specialCommentAndRestOr->second;
                    continue;
                } else if (!specialCommentAndRestOr.error().empty()) {
                    // Error.
                    return std::unexpected(std::move(specialCommentAndRestOr.error()));
                }
                // It's not a special comment starting at beginning of the line but no error either.
                state = State::inMiddleOfLine;
            }
                [[fallthrough]];
            case State::inMiddleOfLine: {
                if (auto tv = TryEatPrefix(sv, "//")) {
                    closeCurrentNonComment();
                    sv = EatWhileNot(*tv, "\n");
                    assert(sv.empty() || sv.front() == '\n');
                    continue;
                } else if ((tv = TryEatPrefix(sv, "/*"))) {
                    closeCurrentNonComment();
                    sv = *tv;
                    bool inComment = true;
                    while (!sv.empty()) {
                        if ((tv = TryEatPrefix(sv, "*/"))) {
                            sv = *tv;
                            inComment = false;
                            break;
                        }
                        sv.remove_prefix(1);
                    }
                    if (inComment) {
                        return std::unexpected("`/*` comment is not closed.");
                    }
                    continue;
                } else if ((tv = TryEatPrefix(sv, "\""))) {
                    makeCurrentNonComment();
                    sv = *tv;
                    for (;;) {
                        tv = EatWhileNot(sv, "\"");
                        sv = *tv;
                        if (sv.empty()) {
                            return std::unexpected("Unclosed double-quote.");
                        } else if (sv.front() == '"') {
                            bool escaped = sv.data()[-1] == '\\';
                            sv.remove_prefix(1);
                            if (escaped) {
                                continue;
                            } else {
                                break;
                            }
                        } else {
                            CHECK(false) << "Unreachable.";
                        }
                    }
                    continue;
                }
                makeCurrentNonComment();
                char c = sv.front();
                sv.remove_prefix(1);
                if (c == '\n') {
                    state = State::atStartOfLine;
                }
            } break;
        }
    }
    closeCurrentNonComment();
    return PreprocessedSource{.specialComments = std::move(specialComments),
                              .nonCommentCode = std::move(nonCommentCode)};
}

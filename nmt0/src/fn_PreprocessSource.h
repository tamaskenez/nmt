// needs: PreprocessedSource, <string_view>, <nonstd/expected.hpp>
nonstd::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv) {
    // needs: EatBlank, TryEatSpecialComment, TryEatPrefix, <glog/logging.h>
    // needs: <utility>, <optional>, EatWhileNot
    enum class State { atStartOfLine, inMiddleOfLine } state = State::atStartOfLine;
    std::string ppSource;
    std::vector<SpecialComment> specialComments;
    while (!sv.empty()) {
        switch (state) {
            case State::atStartOfLine: {
                sv = EatBlank(sv);
                auto maybeSpecialCommentOr = TryEatSpecialComment(sv);
                if (!maybeSpecialCommentOr) {
                    return nonstd::make_unexpected(std::move(maybeSpecialCommentOr.error()));
                }
                if (auto& maybeSpecialComment = *maybeSpecialCommentOr) {
                    sv = maybeSpecialComment->first;
                    specialComments.push_back(
                        SpecialComment{.posInPPSource = ppSource.size(),
                                       .needs = std::move(maybeSpecialComment->second)});
                    continue;
                }
                // It's not a special comment starting at beginning of the line.
                state = State::inMiddleOfLine;
            }
                [[fallthrough]];
            case State::inMiddleOfLine: {
                if (auto tv = TryEatPrefix(sv, "//")) {
                    sv = EatWhileNot(*tv, "\n");
                    assert(sv.empty() || sv.front() == '\n');
                    continue;
                } else if ((tv = TryEatPrefix(sv, "/*"))) {
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
                        return nonstd::make_unexpected("`/*` comment is not closed.");
                    }
                    continue;
                } else if ((tv = TryEatPrefix(sv, "\""))) {
                    ppSource += '\"';
                    sv = *tv;
                    for (;;) {
                        tv = EatWhileNot(sv, "\"");
                        ppSource += sv.substr(0, sv.size() - tv->size());
                        sv = *tv;
                        if (sv.empty()) {
                            return nonstd::make_unexpected("Unclosed double-quote.");
                        } else if (sv.front() == '"') {
                            bool escaped = ppSource.back() == '\\';
                            ppSource += '"';
                            sv.remove_prefix(1);
                            if (escaped) {
                                continue;
                            } else {
                                break;
                            }
                        } else {
                            LOG(FATAL) << "Unreachable.";
                        }
                    }
                    continue;
                }
                char c = sv.front();
                sv.remove_prefix(1);
                if (!isspace(c) || !ppSource.empty()) {  // Skip leading spaces.
                    ppSource += c;
                }
                if (c == '\n') {
                    state = State::atStartOfLine;
                }
            } break;
        }
    }
    return PreprocessedSource{.ppSource = std::move(ppSource),
                              .specialComments = std::move(specialComments)};
}

// needs: PreprocessedSource, <string_view>, <nonstd/expected.hpp>
nonstd::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv) {
    // needs: EatBlank, TryEatSpecialComment, TryEatPrefix, <glog/logging.h>,
    // needs: <utility>, <optional>
    enum class State { atStartOfLine, inMiddleOfLine, inString } state = State::atStartOfLine;
    std::string ppSource;
    std::vector<SpecialComment> specialComments;
    std::optional<std::string> error;
    for (;;) {
        switch (state) {
            case State::atStartOfLine: {
                sv = EatBlank(sv);
                auto maybeSpecialCommentOr = TryEatSpecialComment(sv);
                if (!maybeSpecialCommentOr) {
                    return nonstd::make_unexpected(std::move(maybeSpecialCommentOr.error()));
                }
                if (auto& maybeSpecialComment = *maybeSpecialCommentOr) {
                    sv = maybeSpecialComment->first;
                    specialComments.push_back(std::move(maybeSpecialComment->second));
                    continue;
                }
                state = State::inMiddleOfLine;
            }
                [[fallthrough]];
            case State::inMiddleOfLine: {
                if (auto tv = TryEatPrefix(sv, "//")) {
                    sv = *tv;
                    while (!sv.empty()) {
                        const bool eol = sv[0] == '\n';
                        sv.remove_prefix(1);
                        if (eol) {
                            break;
                        }
                    }
                    state = State::atStartOfLine;
                    continue;
                } else if (auto tv = TryEatPrefix(sv, "/*")) {
                    sv = *tv;
                    bool inComment = true;
                    while (!sv.empty()) {
                        if (auto tv = TryEatPrefix(sv, "*/")) {
                            sv = *tv;
                            inComment = false;
                            break;
                        }
                        sv.remove_prefix(1);
                    }
                    if (inComment) {
                        error = "`/*` comment is not closed in {}";
                    }
                    continue;
                } else if (auto tv = TryEatPrefix(sv, "\"")) {
                    ppSource += '\"';
                    sv = *tv;
                    LOG(FATAL);
                    // ...
                }
                LOG(FATAL);
                // ...
            } break;
            case State::inString:
                LOG(FATAL);
                // ...
                break;
        }
    }
    LOG(FATAL);
    // ...
}

#include "pch.h"

#include "PreprocessSource.h"
#include "TryEatSpecialComment.h"
#include "parse.h"

#include "libtokenizer.h"

std::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv) {
    TRY_ASSIGN(tokens, libtokenizer::process_cpp_with_option_B(sv));
    std::vector<SpecialComment> specialComments;
    bool previousShouldContinue = false;
    for (auto& t : tokens.tokens) {
        if (!t.IsInlineComment()) {
            continue;
        }
        CHECK(t.sourceValue.starts_with("//"));
        auto specialCommentAndRestOr =
            TryEatSpecialCommentAfterSlashSlash(t.sourceValue.substr(2), previousShouldContinue);
        if (specialCommentAndRestOr) {
            // Valid special comment.
            if (previousShouldContinue) {
                auto& back = specialComments.back();
                append_range(back.list, std::move(specialCommentAndRestOr->list));
                back.trailingComma = specialCommentAndRestOr->trailingComma;
            } else {
                specialComments.push_back(std::move(*specialCommentAndRestOr));
            }
            previousShouldContinue = specialComments.back().trailingComma;
        } else if (!specialCommentAndRestOr.error().empty()) {
            // Error.
            return std::unexpected(std::move(specialCommentAndRestOr.error()));
        }
    }
    return PreprocessedSource{.specialComments = std::move(specialComments),
                              .tokens = std::move(tokens)};
}

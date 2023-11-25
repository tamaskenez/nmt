#include "pch.h"

#include "PreprocessSource.h"
#include "Trim.h"
#include "TryEatSpecialComment.h"
#include "parse.h"

#include "libtokenizer.h"

std::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv) {
    TRY_ASSIGN(tokens, libtokenizer::process_cpp_with_option_B(sv));
    std::vector<SpecialComment> specialComments;
    for (auto& t : tokens.tokens) {
        if (!t.IsInlineComment()) {
            continue;
        }
        CHECK(t.sourceValue.starts_with("//"));
        auto specialCommentAndRestOr = TryEatSpecialComment(t.sourceValue);
        if (specialCommentAndRestOr) {
            // Valid special comment.
            specialComments.push_back(std::move(*specialCommentAndRestOr));
        } else if (!specialCommentAndRestOr.error().empty()) {
            // Error.
            return std::unexpected(std::move(specialCommentAndRestOr.error()));
        }
    }
    return PreprocessedSource{.specialComments = std::move(specialComments),
                              .tokens = std::move(tokens)};
}

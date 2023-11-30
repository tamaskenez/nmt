#include "pch.h"

#include "TryEatSpecialComment.h"
#include "enums.h"
#include "parse.h"

std::unordered_set<std::string_view> SpecialCommentKeywords() {
    std::unordered_set<std::string_view> keywords(
        BEGIN_END(enum_traits<SpecialCommentKeyword>::names));
    keywords.insert(BEGIN_END(enum_traits<EntityKind>::names));
    return keywords;
}

const std::unordered_set<std::string_view> k_specialCommentKeywords = SpecialCommentKeywords();

// Eats EOL, too.
struct TryEatCommaSeparateListResult {
    std::vector<std::string_view> items;
    bool trailingComma = false;
};
std::expected<TryEatCommaSeparateListResult, std::string> TryEatCommaSeparatedList(
    std::string_view sv) {
    // Read comma-separated list.
    std::vector<std::string_view> items;
    sv = EatBlank(sv);
    for (;;) {
        if (sv.empty()) {
            break;
        }
        if (sv.front() == '<' || sv.front() == '"') {
            constexpr std::string_view angleBracketChars = "<>\n";
            constexpr std::string_view doubleQuoteChars = "\"\"\n";
            auto chars = sv.front() == '<' ? angleBracketChars : doubleQuoteChars;
            auto tv = EatWhileNot(sv.substr(1), chars.substr(1));
            assert(tv.empty() || chars.substr(1).find(tv.front()) != std::string_view::npos);
            if (tv.empty() || tv.front() != chars[1]) {
                return std::unexpected(fmt::format("'{}' is not closed", chars.substr(0, 2)));
            }
            sv.remove_suffix(tv.size() - 1);
            items.push_back(sv);
            sv = tv.substr(1);
        } else {
            auto tv = EatWhileNot(sv.substr(1), " ,\t\n");
            sv.remove_suffix(tv.size());
            items.push_back(sv);
            sv = tv;
        }
        sv = EatBlank(sv);
        // After a list item we expect:
        // - comma
        // - empty
        if (sv.empty()) {
            break;
        } else if (sv.front() != ',') {
            return std::unexpected(fmt::format("Invalid text in comma-separated list: {}", sv));
        }
        sv.remove_prefix(1);  // Eat comma.
        sv = EatBlank(sv);
        if (sv.empty()) {
            return TryEatCommaSeparateListResult{.items = std::move(items), .trailingComma = true};
        }
    }
    return TryEatCommaSeparateListResult{.items = std::move(items), .trailingComma = false};
}

std::expected<SpecialComment, std::string> TryEatSpecialCommentAfterSlashSlash(
    std::string_view sv, bool previousShouldContinue) {
    std::string_view keyword;
    if (!previousShouldContinue) {
        auto afterPound = TryEatPrefix(EatBlank(sv), "#");
        if (!afterPound) {
            return std::unexpected(std::string());
        }
        auto symbolAndRestOr = TryEatCSymbol(*afterPound);
        if (!symbolAndRestOr) {
            return std::unexpected(std::string());
        }
        std::tie(keyword, sv) = *symbolAndRestOr;
        if (!k_specialCommentKeywords.contains(keyword)) {
            return std::unexpected(std::string());  // Not an error but not a special comment.
        }
        if (sv.empty()) {
            // Special comment without list
            return SpecialComment{.keyword = keyword};
        }
        if (sv.front() != ':') {
            return std::unexpected("Invalid character after special comment.");
        }
        sv.remove_prefix(1);
    }
    TRY_ASSIGN(listResult, TryEatCommaSeparatedList(sv));

    return SpecialComment{.keyword = keyword,
                          .list = std::move(listResult.items),
                          .trailingComma = listResult.trailingComma};
}

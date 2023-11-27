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
std::expected<std::pair<std::vector<std::string_view>, std::string_view>, std::string>
TryEatCommaSeparatedListPossiblyMultiline(std::string_view sv) {
    // Read comma-separated list.
    std::vector<std::string_view> items;
    for (;;) {
        sv = EatBlank(sv);
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
        // - newline
        if (sv.empty() || sv.front() == '\n') {
            if (sv.front() == '\n') {
                sv.remove_prefix(1);
            }
            break;
        }
        if (sv.front() != ',') {
            return std::unexpected("missing comma in list");
        }
        sv.remove_prefix(1);
        // After the comma, the list can continue here or on the next line.
        sv = EatBlank(sv);
        if (sv.empty()) {
            return std::unexpected("Trailing comma is not permitted.");
        }
        if (sv.front() != '\n') {
            continue;  // List continues on this line.
        }
        // List must continue on next line.
        sv.remove_prefix(1);
        auto restOr = TryEatPrefix(EatBlank(sv), "//");
        if (!restOr) {
            return std::unexpected(
                "Missing `//`: after a trailing comma the list should continue on the next line.");
        }
        sv = *restOr;
    }
    return std::make_pair(std::move(items), sv);
}

std::expected<SpecialComment, std::string> TryEatSpecialComment(std::string_view sv) {
    sv = trim(sv);
    auto afterSlashSlash = TryEatPrefix(sv, "//");
    if (!afterSlashSlash) {
        return std::unexpected(std::string());
    }
    auto afterPound = TryEatPrefix(EatBlank((*afterSlashSlash)), "#");
    if (!afterPound) {
        return std::unexpected(std::string());
    }
    auto symbolAndRestOr = TryEatCSymbol(*afterPound);
    if (!symbolAndRestOr) {
        return std::unexpected(std::string());
    }
    std::string_view keyword;
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
    auto listAndRestOr = TryEatCommaSeparatedListPossiblyMultiline(sv);
    if (!listAndRestOr) {
        return std::unexpected(listAndRestOr.error());
    }

    return SpecialComment{.keyword = keyword, .list = listAndRestOr->first};
}

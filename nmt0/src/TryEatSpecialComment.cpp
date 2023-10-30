#include "pch.h"

#include "TryEatSpecialComment.h"
#include "parse.h"

// Eats EOL, too.

const static inline std::unordered_set<std::string_view> k_specialCommentKeywords = {
    "fdneeds",   // forward-declaration needs
    "oedneeds",  // opaque-enum-declaration needs
    "needs",     // declaration needs
    "defneeds",  // definition needs
    "namespace",
    "visibility"};

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

std::expected<std::pair<SpecialComment, std::string_view>, std::string> TryEatSpecialComment(
    std::string_view sv) {
    auto afterSlashSlash = TryEatPrefix(sv, "//");
    if (!afterSlashSlash) {
        return std::unexpected(std::string());
    }
    auto symbolAndRestOr = TryEatCSymbol(EatBlank(*afterSlashSlash));
    if (!symbolAndRestOr) {
        return std::unexpected(std::string());
    }
    std::string_view keyword;
    std::tie(keyword, sv) = *symbolAndRestOr;
    if (!k_specialCommentKeywords.contains(keyword) || sv.empty() || sv.front() != ':') {
        return std::unexpected(std::string());
    }
    sv.remove_prefix(1);
    auto listAndRestOr = TryEatCommaSeparatedListPossiblyMultiline(sv);
    if (!listAndRestOr) {
        return std::unexpected(listAndRestOr.error());
    }

    return std::make_pair(SpecialComment{.keyword = keyword, .list = listAndRestOr->first},
                          listAndRestOr->second);
}

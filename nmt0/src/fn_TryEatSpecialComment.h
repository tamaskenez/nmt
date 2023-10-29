// Eats EOL, too.

// needs: <expected>, <string>, <string_view>, <optional>, <utility>
std::expected<std::optional<std::pair<std::string_view, std::vector<std::string>>>, std::string>
TryEatSpecialComment(std::string_view sv)
// needs: TryEatPrefix, EatBlank, EatWhileNot, <glog/logging.h>, <fmt/core.h>
{
    auto afterSlashSlash = TryEatPrefix(sv, "//");
    if (!afterSlashSlash) {
        return std::nullopt;
    }
    auto afterNeeds = TryEatPrefix(EatBlank(*afterSlashSlash), "needs:");
    if (!afterNeeds) {
        return std::nullopt;
    }
    // Read comma-separated list.
    std::vector<std::string> items;
    sv = *afterNeeds;
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
            CHECK(tv.empty() || chars.substr(1).find(tv.front()) != std::string_view::npos);
            if (tv.empty() || tv.front() != chars[1]) {
                return std::unexpected(fmt::format(
                    "'// needs:' comment contains unclosed '{}' item", chars.substr(0, 2)));
            }
            sv.remove_suffix(tv.size() - 1);
            items.push_back(std::string(sv));
            sv = tv.substr(1);
        } else {
            auto tv = EatWhileNot(sv.substr(1), " ,\t\n");
            sv.remove_suffix(tv.size());
            items.push_back(std::string(sv));
            sv = tv;
        }
        sv = EatBlank(sv);
        if (sv.empty() || sv.front() == '\n') {
            if (sv.front() == '\n') {
                sv.remove_prefix(1);
            }
            break;
        }
        if (sv.front() != ',') {
            return std::unexpected("'needs: ' missing comma in list");
        }
        sv.remove_prefix(1);
    }
    return std::make_optional(std::make_pair(sv, std::move(items)));
}

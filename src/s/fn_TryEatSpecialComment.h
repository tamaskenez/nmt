// Eats EOL, too.

// needs: <nonstd/expected.hpp>, SpecialComment, <string>, <string_view>, <optional>, <utility>
nonstd::expected<std::optional<std::pair<std::string_view, SpecialComment>>, std::string>
TryEatSpecialComment(std::string_view sv)
// needs: TryEatPrefix, EatBlank, EatWhileNot, <glog/logging.h>
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
    for (;;) {
        sv = EatBlank(*afterNeeds);
        if (sv.empty()) {
            break;
        }
        if (sv.front() == '<') {
            sv.remove_prefix(1);
            constexpr std::string_view terminatingChars = ">\n";
            auto tv = EatWhileNot(sv, terminatingChars);
            CHECK(tv.empty() || terminatingChars.find(tv.front()) != std::string_view::npos);
            if (tv.empty() || tv.front() != '>') {
                return nonstd::make_unexpected("'// needs:' comment contains unclosed '<>' item");
            }
            sv.remove_suffix(sv.size() - tv.size());
            items.push_back(std::string(sv));
            sv = tv;
        } else if (sv.front() == '\"') {
            sv.remove_prefix(1);
            LOG(FATAL);
            // ...
        } else {
            LOG(FATAL);
            // ...
        }
        LOG(FATAL);
        // ...
    }
    return std::nullopt;
}

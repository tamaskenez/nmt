#include "parse.h"

std::expected<std::pair<std::string, std::string>, std::string> ParseStructClassDeclaration(
    std::string_view sv) {
    // - Find the first `{` or `:`, whichever is earlier
    // - before that it's <NAME>(final)?
    auto sv0 = EatBlank(sv);
    auto firstOpeningCurlyBracket = sv0.find('{');
    if (firstOpeningCurlyBracket == std::string_view::npos) {
        return std::unexpected("Can't find first opening curly bracket.");
    }
    auto firstColon = sv0.find(':');
    auto nameFinalEndPos =
        firstColon != std::string_view::npos && firstColon < firstOpeningCurlyBracket
            ? firstColon
            : firstOpeningCurlyBracket;

    std::optional<std::string> finalName;
    for ([[maybe_unused]] int pass : {1, 2}) {
        auto tv = Trim(sv0.substr(0, nameFinalEndPos));

        auto i = tv.size();
        while (i > 0 && (isalnum(tv[i - 1]) || tv[i - 1] == '_')) {
            --i;
        }
        finalName = ExtractIdentifier(tv.substr(i));

        if (!finalName) {
            return std::unexpected("Can't find struct/class name");
        }
        if (finalName != "final") {
            break;
        }
        finalName.reset();
        nameFinalEndPos = i;
    }

    auto finalDeclaration = CompactSpaces(sv0.substr(0, firstOpeningCurlyBracket));

    return std::make_pair(std::string(finalName.value()), std::move(finalDeclaration));
}

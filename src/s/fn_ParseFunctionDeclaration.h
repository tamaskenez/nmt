// needs: <string_view>, <nonstd/expected.hpp>, <utility>, <string>
nonstd::expected<std::pair<std::string, std::string>, std::string> ParseFunctionDeclaration(
    std::string_view sv)
// needs: EatBlank
// needs: ExtractIdentifier, <fmt/std.h>, Trim
// needs: CompactSpaces
{
    // - Find the identifier before the first `(`, that will be the name. Note that this will fail
    // if the return type contains parentheses which can happen.
    // - Find the first `{` for the entire declaration.
    auto sv0 = EatBlank(sv);
    auto firstOpeningParenthesis = sv0.find('(');
    if (firstOpeningParenthesis == std::string_view::npos) {
        return nonstd::make_unexpected("Can't find first opening parentheses.");
    }
    auto firstOpeningCurlyBracket = sv0.find('{');
    if (firstOpeningParenthesis == std::string_view::npos) {
        return nonstd::make_unexpected("Can't find first opening curly bracket.");
    }
    if (firstOpeningParenthesis >= firstOpeningCurlyBracket) {
        return nonstd::make_unexpected(
            "First opening curly bracket found before the first opening parentheses.");
    }
    auto tv = Trim(sv0.substr(0, firstOpeningParenthesis));
    auto i = tv.size();
    while (i > 0 && (isalnum(tv[i - 1]) || tv[i - 1] == '_')) {
        --i;
    }
    auto finalName = ExtractIdentifier(tv.substr(i));
    if (!finalName) {
        return nonstd::make_unexpected(fmt::format("Can't extract function name from `{}`", tv));
    }
    auto finalDeclaration = CompactSpaces(sv0.substr(0, firstOpeningCurlyBracket));

    return std::make_pair(std::string(*finalName), std::move(finalDeclaration));
}

// needs: <string_view>, <nonstd/expected.hpp>, <utility>, <string>
nonstd::expected<std::pair<std::string, std::string>, std::string> ParseFunctionDeclaration(
    std::string_view sv)
// needs:
{
    (void)sv;
    return nonstd::make_unexpected("");
}

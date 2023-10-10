// needs: <string_view>, <nonstd/expected.hpp>, <utility>, <string>
nonstd::expected<std::pair<std::string, std::string>, std::string> ParseStructClassDeclaration(
    std::string_view sv)
// needs:
{
    (void)sv;
    return nonstd::make_unexpected("");
}

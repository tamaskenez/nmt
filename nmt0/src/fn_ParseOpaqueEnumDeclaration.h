// needs: <string_view>, <expected>, <utility>, <string>
std::expected<std::pair<std::string, std::string>, std::string> ParseOpaqueEnumDeclaration(
    std::string_view sv)
// needs: EatBlank, TryEatPrefix, EatSpace, EatWhileNot
// needs: ExtractIdentifier, <fmt/std.h>, Trim
// needs: CompactSpaces
{
    // Find the "opaque enum declaration" part of the source:
    //     enum(class|struct)?([[...]])?<NAME>(:<BASE>)?{
    auto sv0 = EatBlank(sv);
    auto tv = TryEatPrefix(sv0, "enum");
    if (!tv) {
        return std::unexpected("First word must be enum.");
    }
    sv = *tv;
    if (sv.empty()) {
        return std::unexpected("Missing declaration after `enum`.");
    }
    bool hadSeparator = false;
    if (isspace(sv.front())) {
        sv = EatSpace(sv);
        if ((tv = TryEatPrefix(sv, "struct"))) {
            sv = EatSpace(*tv);
            hadSeparator = sv.size() < tv->size();
        } else if ((tv = TryEatPrefix(sv, "class"))) {
            sv = EatSpace(*tv);
            hadSeparator = sv.size() < tv->size();
        }
    }
    if ((tv = TryEatPrefix(sv, "[["))) {
        sv = *tv;
        bool found = false;
        while (!sv.empty()) {
            if ((tv = TryEatPrefix(sv, "]]"))) {
                sv = *tv;
                found = true;
                break;
            }
            sv.remove_prefix(1);
        }
        if (!found) {
            return std::unexpected("Unclosed `[[` in `enum`.");
        }
        hadSeparator = true;
    }
    if (!hadSeparator) {
        return std::unexpected("Invalid `enum` declaration or missing name.");
    }

    auto uv = EatWhileNot(sv, ":{");
    if (uv.empty()) {
        return std::unexpected("Missing declaration after enum name.");
    }
    auto rawName = sv.substr(0, sv.size() - uv.size());
    auto finalName = ExtractIdentifier(rawName);
    if (!finalName) {
        return std::unexpected(fmt::format("Invalid enum name: {}", Trim(rawName)));
    }
    if (uv.front() == ':') {
        uv = EatWhileNot(uv, "{");
        if (uv.empty()) {
            return std::unexpected("Missing declaration after enum name.");
        }
    } else {
        assert(uv.front() == '{');
    }
    auto finalDeclaration = CompactSpaces(sv0.substr(0, sv0.size() - uv.size()));
    return std::make_pair(std::string(*finalName), std::move(finalDeclaration));
}

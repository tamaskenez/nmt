// needs: <string_view>, <string>
std::string CompactSpaces(std::string_view sv)
// needs: <cctype>, <cassert>, Trim
{
    sv = Trim(sv);
    std::string r;
    r.reserve(sv.size());
    for (auto c : sv) {
        if (isspace(c)) {
            assert(!r.empty());  // Because of Trim().
            if (r.back() != ' ') {
                r += ' ';
            }
        } else {
            r += c;
        }
    }
    return r;
}
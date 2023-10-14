// needs: <string_view>
std::string_view Trim(std::string_view sv)
// needs: <cctype>
{
    while (!sv.empty() && isspace(sv.front())) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && isspace(sv.back())) {
        sv.remove_suffix(1);
    }
    return sv;
}

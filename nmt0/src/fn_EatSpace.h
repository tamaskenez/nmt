// needs: <string_view>
std::string_view EatSpace(std::string_view sv)
// needs: <cctype>
{
    while (!sv.empty() && isspace(sv.front())) {
        sv.remove_prefix(1);
    }
    return sv;
}

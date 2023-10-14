// needs: <string_view>
std::string_view EatBlank(std::string_view sv)
// needs: <cctype>
{
    while (!sv.empty() && isblank(sv.front())) {
        sv.remove_prefix(1);
    }
    return sv;
}

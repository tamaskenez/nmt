// needs: <string_view>
std::string_view EatWhileNot(std::string_view sv, std::string_view chs) {
    while (!sv.empty() && chs.find(sv.front()) != std::string_view::npos) {
        sv.remove_prefix(1);
    }
    return sv;
}

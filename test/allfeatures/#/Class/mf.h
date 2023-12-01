// The first comment
/* Second comment */
// #memfn
Class::int_type Class::mf(const std::set<int>& k) const {
    std::vector<int> v(k.begin(), k.end());
    return 2 * int(v.size());
}
// #needs: <set>
// #defneeds: <vector>

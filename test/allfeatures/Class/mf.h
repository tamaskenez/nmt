// The first comment
/* Second comment */
// #memfn
Class::int_type Class::mf(const std::set<int>& k) {
    std::vector<int> v(k.begin(), k.end());
    return 2 * v.size();
}
// #needs: <set>
// #defneeds: <vector>

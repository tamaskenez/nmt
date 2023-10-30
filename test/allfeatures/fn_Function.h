namespace {
const int localVar = 123;
}

// #needs: <optional>, Enum*, Struct*, Class*
// #defneeds: Enum, Struct, <cstdio>,
// Class, <utility>, <set>
std::optional<int> Function(Enum e, const Class& c, const Struct& s) {
    auto a =
        c.a(std::set<int>({1, 2, 3})) + c.s.size() + s.v.size() + std::to_underlying(Enum::Three);
    printf("%d %d\n", a, localVar);
    return a;
}
namespace {
const int localVar = 123;
}

// #fn
std::optional<int> Function(Enum e, const Class& c, const Struct& s) {
    auto a = size_t(c.mf(std::set<int>({1, 2, 3}))) + c.s.size() + s.v.size()
           + size_t(std::to_underlying(Enum::Three));
    printf("%zu %d %d\n", a, localVar, std::to_underlying(e));
    return a;
}

// #needs: <optional>, Enum*, Struct*, Class*
// #defneeds: Enum, Struct, <cstdio>,
// Class, <utility>, <set>

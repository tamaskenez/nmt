#pragma once

enum class EntityKind {
    enum_,
    fn,
    struct_,
    class_,
    using_,
    inlvar,
};
template<>
struct enum_traits<EntityKind> {
    using enum EntityKind;
    static constexpr std::array<EntityKind, 6> elements{enum_, fn, struct_, class_, using_, inlvar};
    static constexpr std::array<std::string_view, elements.size()> names{
        "enum", "fn", "struct", "class", "using", "inlvar"};
};

// Must be EntityKind extended.
enum class EntityKindOrSub { enum_, fn, struct_, class_, using_, inlvar, memfn };
template<>
struct enum_traits<EntityKindOrSub> {
    using enum EntityKindOrSub;
    static constexpr std::array<EntityKindOrSub, 7> elements{
        enum_, fn, struct_, class_, using_, inlvar, memfn};
    static constexpr std::array<std::string_view, elements.size()> names{
        "enum", "fn", "struct", "class", "using", "inlvar", "memfn"};
};

enum class Visibility { public_, target };
template<>
struct enum_traits<Visibility> {
    using enum Visibility;
    static constexpr std::array<Visibility, 2> elements{public_, target};
    static constexpr std::array<std::string_view, elements.size()> names{"public", "target"};
};

enum class SpecialCommentKeyword { fdneeds, needs, defneeds, visibility };
template<>
struct enum_traits<SpecialCommentKeyword> {
    using enum SpecialCommentKeyword;
    static constexpr std::array<SpecialCommentKeyword, 4> elements{
        fdneeds, needs, defneeds, visibility};
    static constexpr std::array<std::string_view, elements.size()> names{
        "fdneeds",   // forward-declaration (and opaque-enum-declaration) needs
        "needs",     // declaration needs
        "defneeds",  // definition needs
        "visibility"};
};

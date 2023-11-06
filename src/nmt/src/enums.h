#pragma once

enum class EntityKind {
    enum_,
    fn,
    struct_,
    class_,
    using_,
    inlvar,
    memfn,
};
template<>
struct enum_traits<EntityKind> {
    using enum EntityKind;
    static constexpr std::array<EntityKind, 7> elements{
        enum_, fn, struct_, class_, using_, inlvar, memfn};
    static constexpr std::array<std::string_view, elements.size()> names{
        "enum", "fn", "struct", "class", "using", "inlvar", "memfn"};
};

enum class NeedsKind { forwardDeclaration, opaqueEnumDeclaration, definition, declaration };
template<>
struct enum_traits<NeedsKind> {
    using enum NeedsKind;
    static constexpr std::array<NeedsKind, 4> elements{
        forwardDeclaration, opaqueEnumDeclaration, definition, declaration};
    static constexpr std::array<std::string_view, elements.size()> names{
        "forwardDeclaration", "opaqueEnumDeclaration", "definition", "declaration"};
};

enum class Visibility { public_, target };
template<>
struct enum_traits<Visibility> {
    using enum Visibility;
    static constexpr std::array<Visibility, 2> elements{public_, target};
    static constexpr std::array<std::string_view, elements.size()> names{"public", "target"};
};

enum class SpecialCommentKeyword {
    entity,
    fdneeds,
    oedneeds,
    needs,
    defneeds,
    namespace_,
    visibility
};
template<>
struct enum_traits<SpecialCommentKeyword> {
    using enum SpecialCommentKeyword;
    static constexpr std::array<SpecialCommentKeyword, 7> elements{
        entity, fdneeds, oedneeds, needs, defneeds, namespace_, visibility};
    static constexpr std::array<std::string_view, elements.size()> names{
        "entity",
        "fdneeds",   // forward-declaration needs
        "oedneeds",  // opaque-enum-declaration needs
        "needs",     // declaration needs
        "defneeds",  // definition needs
        "namespace",
        "visibility"};
};

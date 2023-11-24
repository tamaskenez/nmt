// needs: <string_view>, EntityKind*
std::string_view EntityKindLongName(EntityKind ek)
// needs: EntityKind
{
    switch (ek) {
        case EntityKind::enum_:
            return "enum";
        case EntityKind::fn:
            return "function";
        case EntityKind::struct_:
            return "struct";
        case EntityKind::class_:
            return "class";
        case EntityKind::using_:
            return "using";
        case EntityKind::inlvar:
            return "inlvar";
    }
}

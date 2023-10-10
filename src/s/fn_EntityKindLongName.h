// needs: <string_view>, EntityKind*
std::string_view EntityKindLongName(EntityKind ek)
// needs: EntityKind;
{
    switch (ek) {
        case EntityKind::enum_:
            return "enum";
        case EntityKind::fn:
            return "function";
        case EntityKind::sc:
            return "struct/class";
    }
}
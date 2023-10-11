// needs: <string_view>, EntityKind*
std::string_view EntityKindShortName(EntityKind ek)
// needs: EntityKind;
{
    switch (ek) {
        case EntityKind::enum_:
            return "enum";
        case EntityKind::fn:
            return "fn";
        case EntityKind::sc:
            return "sc";
    }
}
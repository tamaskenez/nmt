// needs: EntityKind*, <string>, <filesystem>, <vector>
struct Entity {
    EntityKind kind;
    std::string name;
    // lightDeclaration is
    // - opaque-enum-declaration for enums
    // - forward-declaration for structs, classes
    // - function-declaration for functions
    std::string lightDeclaration;
    std::filesystem::path path;
    std::vector<std::string> declNeeds, defNeeds;
};

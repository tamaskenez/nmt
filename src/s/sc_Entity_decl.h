struct Entity {
    std::string name;
    EntityKind entityKind;
    std::string fwd;
    struct Dependencies {
        std::vector<std::string> includes;
        std::vector<std::string> fwds;
        std::vector<std::string> decls;
    } declDeps std::optional<defDeps>;
};

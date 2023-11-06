#pragma once

#include "enums.h"

struct SpecialComment {
    std::string_view keyword;
    std::vector<std::string_view> list;
};
struct PreprocessedSource {
    std::vector<SpecialComment> specialComments;
    std::vector<std::string_view> nonCommentCode;
};

struct EntityProperties {
    static constexpr Visibility k_defaultVisibility = Visibility::target;

    EntityKind entityKind;  // No default value.
    absl::flat_hash_map<NeedsKind, std::vector<std::string>> needsByKind;
    std::optional<std::string> namespace_;
    Visibility visibility = Visibility::target;

    void Print() {
        for (auto k : enum_traits<NeedsKind>::elements) {
            auto it = needsByKind.find(k);
            if (it == needsByKind.end()) {
                continue;
            }
            fmt::print("needs for {}: {}\n", enum_name<NeedsKind>(k), fmt::join(it->second, ", "));
        }
        if (namespace_) {
            fmt::print("namespace: {}\n", *namespace_);
        }
        fmt::print("visibility: {}\n", enum_name<Visibility>(visibility));
    }
};

struct Entity {
    std::string name;
    std::filesystem::path path;
    EntityProperties props;

    // lightDeclaration is
    // - opaque-enum-declaration for enums
    // - forward-declaration for structs, classes
    // - function-declaration for functions
    std::optional<std::string> lightDeclaration;
};
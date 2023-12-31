#pragma once

#include "nmt/base_types.h"
#include "nmt/constants.h"
#include "nmt/enums.h"
#include "util/stlext.h"

#include <fmt/core.h>

#include <string>
#include <variant>
#include <vector>

struct MemberFunction {};

namespace EntityDependentProperties {
struct Enum {
    // opaque enum declaration classifies as a kind of forward declaration.
    std::string opaqueEnumDeclaration;
    // The source file contains the enum-declaration;
    std::vector<std::string> opaqueEnumDeclarationNeeds;
    std::vector<std::string> declarationNeeds;
};
struct Fn {
    std::string declaration;
    // The source file contains the function-definition.
    std::vector<std::string> declarationNeeds, definitionNeeds;
};
struct StructOrClass {
    std::string forwardDeclaration;
    // The source file contains the struct/class declaration with an optional, special macro to
    // inject the member declarations.
    std::vector<std::string> forwardDeclarationNeeds, declarationNeeds;
    flat_hash_map<std::string, MemberFunction> memberFunctions;
};
struct Header {
    // The source file contains a header-only entity, like type alias (using) or inline variable.
    std::vector<std::string> declarationNeeds;
};
struct MemFn {
    std::string declaration;
    // The source file contains the function-definition.
    std::vector<std::string> declarationNeeds, definitionNeeds;
};
using V = std::variant<Enum, Fn, StructOrClass, StructOrClass, Header, MemFn>;
// Make sure V's alternatives correspond to EntityKind values.
// This function doesn't need to be called, it asserts in comptime.
inline void AssertVariantIndices(EntityKind k) {
    using enum EntityKind;
    static_assert(std::variant_size_v<V> == enum_size<EntityKind>());
    switch (k) {
#define CHECK_ALTERNATIVE(e, E)                                                                 \
    case e:                                                                                     \
        static_assert(std::is_same_v<std::variant_alternative_t<std::to_underlying(e), V>, E>); \
        break
        CHECK_ALTERNATIVE(enum_, Enum);
        CHECK_ALTERNATIVE(fn, Fn);
        CHECK_ALTERNATIVE(struct_, StructOrClass);
        CHECK_ALTERNATIVE(class_, StructOrClass);
        CHECK_ALTERNATIVE(header, Header);
        CHECK_ALTERNATIVE(memfn, MemFn);
#undef CHECK_ALTERNATIVE
    }
}
};  // namespace EntityDependentProperties

struct Entity {
    static constexpr Visibility k_defaultVisibility = Visibility::private_;

    int64_t targetId = 0;
    std::string name;  // Unqualified name.
    std::filesystem::path sourcePath;
    std::filesystem::path sourceRelPath;
    std::filesystem::file_time_type lastWriteTime = std::filesystem::file_time_type::min();
    std::optional<std::string> namespace_;
    Visibility visibility = Visibility::private_;

    EntityDependentProperties::V dependentProps;

    void Print();

    EntityKind GetEntityKind() const {
        return from_underlying<EntityKind>(int(dependentProps.index())).value();
    }
    const std::vector<std::string>* ForwardDeclarationNeedsOrNull() const;
    std::string_view ForwardDeclaration() const;
};

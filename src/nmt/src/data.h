#pragma once

#include "enums.h"

#include "constants.h"
#include "libtokenizer.h"

#ifdef NDEBUG
template<class K, class V, class H = absl::flat_hash_map<K, V>::hasher>
using flat_hash_map = absl::flat_hash_map<K, V, H>;
template<class K,
         class H = absl::flat_hash_set<K>::hasher using flat_hash_set = absl::flat_hash_set<K, H>;
#else
template<class K, class V, class H = std::unordered_map<K, V>::hasher>
using flat_hash_map = std::unordered_map<K, V, H>;
template<class K, class H = std::unordered_set<K>::hasher>
using flat_hash_set = std::unordered_set<K, H>;
#endif

struct SpecialComment {
    std::string_view keyword;  // Points into the original source text.
    std::vector<std::string_view> list;
    bool trailingComma =
        false;  // Used only when parsing, one specialcomment can continue in the next line.
};
struct PreprocessedSource {
    std::vector<SpecialComment> specialComments;
    libtokenizer::Result tokens;
};

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
#if 0  // TODO unused
inline V Make(EntityKind k) {
    using enum EntityKind;
    switch (k) {
#    define MAKE_ALTERNATIVE(e) \
        case e:                 \
            return V(std::in_place_index<std::to_underlying(e)>)
        MAKE_ALTERNATIVE(enum_);
        MAKE_ALTERNATIVE(fn);
        MAKE_ALTERNATIVE(struct_);
        MAKE_ALTERNATIVE(class_);
        MAKE_ALTERNATIVE(header);
			MAKE_ALTERNATIVE(memfn);
#    undef MAKE_ALTERNATIVE
    }
}
#endif
};  // namespace EntityDependentProperties

struct Entity {
    static constexpr Visibility k_defaultVisibility = Visibility::private_;

    std::string name;            // Unqualified name.
    std::filesystem::path path;  // Path of the source file. TODO rename to sourcePath.
    std::optional<std::string> namespace_;
    Visibility visibility = Visibility::private_;

    EntityDependentProperties::V dependentProps;

    std::string HeaderFilename() const {
        return fmt::format("{}.h", name);
    }
    // The file which is injected into a class/struct declaration.
    std::string MemberDeclarationsFilename() const {
        return fmt::format("{}#member_declarations.h", name);
    }
    std::string DefinitionFilename() const {
        return fmt::format("{}.cpp", name);
    }
    std::string MemFnClassName() const {
        CHECK(GetEntityKind() == EntityKind::memfn);
        return path_to_string(path.parent_path().filename());
    }

    std::filesystem::path VisibilitySubdir() const {
        switch (visibility) {
            case Visibility::public_:
                return k_publicSubdir;
            case Visibility::private_:
                return k_privateSubdir;
        }
    }
    std::filesystem::path HeaderSubdir() const {
        return {};
    }
    std::filesystem::path HeaderPath() const {
        return VisibilitySubdir() / HeaderSubdir() / path_from_string(HeaderFilename());
    }
    std::filesystem::path MemberDeclarationsPath() const {
        return VisibilitySubdir() / HeaderSubdir() / path_from_string(MemberDeclarationsFilename());
    }
    std::filesystem::path DefinitionPath() const {
        return k_privateSubdir / HeaderSubdir() / path_from_string(DefinitionFilename());
    }

    void Print() {
        fmt::print("entityKind: {}\n", enum_name(GetEntityKind()));
        switch_variant(
            dependentProps,
            [](const EntityDependentProperties::Enum& x) {
                fmt::print("opaque enum declaration: {}\n", x.opaqueEnumDeclaration);
                fmt::print("fdneeds: {}\n", fmt::join(x.opaqueEnumDeclarationNeeds, ", "));
                fmt::print("needs: {}\n", fmt::join(x.declarationNeeds, ", "));
            },
            [](const EntityDependentProperties::Fn& x) {
                fmt::print("declaration: {}\n", x.declaration);
                fmt::print("needs: {}\n", fmt::join(x.declarationNeeds, ", "));
                fmt::print("defNeeds: {}\n", fmt::join(x.definitionNeeds, ", "));
            },
            [](const EntityDependentProperties::StructOrClass& x) {
                fmt::print("forward declaration: {}\n", x.forwardDeclaration);
                fmt::print("fdneeds: {}\n", fmt::join(x.forwardDeclarationNeeds, ", "));
                fmt::print("needs: {}\n", fmt::join(x.declarationNeeds, ", "));
                fmt::print("{} member functions\n", x.memberFunctions.size());
            },
            [](const EntityDependentProperties::Header& x) {
                fmt::print("needs: {}\n", fmt::join(x.declarationNeeds, ", "));
            },
            [](const EntityDependentProperties::MemFn& x) {
                fmt::print("declaration: {}\n", x.declaration);
                fmt::print("needs: {}\n", fmt::join(x.declarationNeeds, ", "));
                fmt::print("defNeeds: {}\n", fmt::join(x.definitionNeeds, ", "));
            });
        if (namespace_) {
            fmt::print("namespace: {}\n", *namespace_);
        }
        fmt::print("visibility: {}\n", enum_name<Visibility>(visibility));
    }

    EntityKind GetEntityKind() const {
        return from_underlying<EntityKind>(int(dependentProps.index())).value();
    }
    const std::vector<std::string>* ForwardDeclarationNeedsOrNull() const {
        using enum EntityKind;
        switch (GetEntityKind()) {
            case enum_:
                return &std::get<std::to_underlying(enum_)>(dependentProps)
                            .opaqueEnumDeclarationNeeds;
            case struct_:
                return &std::get<std::to_underlying(struct_)>(dependentProps)
                            .forwardDeclarationNeeds;
            case class_:
                return &std::get<std::to_underlying(class_)>(dependentProps)
                            .forwardDeclarationNeeds;
            case fn:
            case header:
            case memfn:
                return nullptr;
        }
    }
    std::string_view ForwardDeclaration() const {
        using enum EntityKind;
        switch (GetEntityKind()) {
            case enum_:
                return std::get<std::to_underlying(enum_)>(dependentProps).opaqueEnumDeclaration;
            case struct_:
                return std::get<std::to_underlying(struct_)>(dependentProps).forwardDeclaration;
            case class_:
                return std::get<std::to_underlying(class_)>(dependentProps).forwardDeclaration;
            case fn:
            case header:
            case memfn:
                LOG(FATAL) << fmt::format("Entity {} has no forward declaration",
                                          enum_name(GetEntityKind()));
        }
    }
};

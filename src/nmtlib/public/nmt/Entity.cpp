#include "nmt/Entity.h"

void Entity::Print() {
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

const std::vector<std::string>* Entity::ForwardDeclarationNeedsOrNull() const {
    using enum EntityKind;
    switch (GetEntityKind()) {
        case enum_:
            return &std::get<std::to_underlying(enum_)>(dependentProps).opaqueEnumDeclarationNeeds;
        case struct_:
            return &std::get<std::to_underlying(struct_)>(dependentProps).forwardDeclarationNeeds;
        case class_:
            return &std::get<std::to_underlying(class_)>(dependentProps).forwardDeclarationNeeds;
        case fn:
        case header:
        case memfn:
            return nullptr;
    }
}
std::string_view Entity::ForwardDeclaration() const {
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

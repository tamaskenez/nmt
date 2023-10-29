#pragma once
enum class EntityKind;

std::optional<EntityKind> EntityKindOfStem(std::string_view stem);

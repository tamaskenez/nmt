enum class EntityKind;

#include <optional>
#include <string>

std::optional<EntityKind> EntityKindOfStem(std::string_view stem);

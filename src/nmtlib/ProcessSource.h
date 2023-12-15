#include "nmt/Entity.h"

struct ProcessSourceResult {
    std::optional<Entity> entity;
    std::vector<std::string> messages;
};
std::expected<ProcessSourceResult, std::string> ProcessSource(
    const std::filesystem::path& sourcePath, bool verbose);

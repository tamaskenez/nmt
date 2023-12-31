#include "nmt/DirConfigFile.h"
#include "nmt/Entities.h"
#include "nmt/Entity.h"

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

struct Project;

namespace ProcessSourceResult {
struct SourceWithoutSpecialComments {};
struct CantReadFile {};
struct Error {
    std::vector<std::string> messages;
};
using V = std::variant<Entity, DirConfigFile, SourceWithoutSpecialComments, CantReadFile, Error>;
}  // namespace ProcessSourceResult
ProcessSourceResult::V ProcessSource(int64_t targetId,
                                     const std::filesystem::path& targetRootSourceDir,
                                     const std::filesystem::path& sourcePath);
std::pair<std::vector<std::string>, std::vector<std::string>> ProcessSourceAndUpdateProject(
    Project& project, Entities::Id id, bool verbose);

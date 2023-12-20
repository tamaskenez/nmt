#include "nmt/Entity.h"

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace ProcessSourceResult {
struct SourceWithoutSpecialComments {
    std::filesystem::file_time_type lastWriteTime;
};
struct CantReadFile {};
struct Error {
    std::string message;
    std::filesystem::file_time_type lastWriteTime;
};
using V = std::variant<Entity, SourceWithoutSpecialComments, CantReadFile, Error>;
}  // namespace ProcessSourceResult
ProcessSourceResult::V ProcessSource(const std::filesystem::path& sourcePath);

#pragma once

#include <filesystem>
#include <set>
#include <string_view>
#include <vector>

constexpr std::string_view k_autogeneratedWarningLine =
    "// AUTOGENERATED FILE, EDITS WILL BE OVERWRITTEN!";
constexpr std::string_view k_IntentionallyEmptyLineWarning = "// Intentionally empty.";

constexpr std::string_view k_emptyHeaderFilename = "#empty.h";
constexpr std::string_view k_fileListFilename = "files.txt";

inline const std::set<std::filesystem::path> k_validSourceExtensions = {".h", ".hpp", ".hxx"};

constexpr std::string_view k_nmtIncludeMemberDeclarationsMacro = "NMT_MEMBER_DECLARATIONS";

inline const std::filesystem::path k_publicSubdir = "public";
inline const std::filesystem::path k_privateSubdir = "private";
constexpr std::string_view k_memberDeclarationsFilenamePostfix = "#memberDecls";
constexpr std::string_view k_membersDirPostfix = "#members";

struct MemberDefinitionMacro {
    std::string_view name;
    std::string_view forHeader;
    std::string_view forCpp;
};
inline const std::vector<MemberDefinitionMacro> k_memberDefinitionMacros = {
    MemberDefinitionMacro{"OVERRIDE", "override", ""},
    MemberDefinitionMacro{"VIRTUAL", "virtual", ""},
    MemberDefinitionMacro{"EXPLICIT", "explicit", ""},
    MemberDefinitionMacro{"STATIC", "static", ""}};
constexpr std::string_view k_dirConfigFileName = "#";

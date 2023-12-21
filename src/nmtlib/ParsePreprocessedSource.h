#pragma once

#include "data.h"
#include "nmt/DirConfigFile.h"
#include "nmt/Entity.h"
#include "nmt/enums.h"

struct ParsePreprocessedSourceResult {
    std::string name;
    std::optional<Visibility> visibility;
    EntityDependentProperties::V dependentProps;
};
std::expected<ParsePreprocessedSourceResult, std::vector<std::string>> ParsePreprocessedSource(
    const PreprocessedSource& pps, const std::filesystem::path& sourcePath);
std::expected<DirConfigFile, std::vector<std::string>> ParseDirConfigFile(
    const std::vector<SpecialComment>& specialComments, const std::filesystem::path& sourcePath);

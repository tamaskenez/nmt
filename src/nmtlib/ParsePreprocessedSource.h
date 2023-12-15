#pragma once

#include "data.h"
#include "nmt/Entity.h"
#include "nmt/enums.h"

struct ParsePreprocessedSourceResult {
    std::optional<Visibility> visibility;
    EntityDependentProperties::V dependentProps;
};
std::expected<ParsePreprocessedSourceResult, std::string> ParsePreprocessedSource(
    std::string_view name, const PreprocessedSource& pps, std::string_view parentDirName);

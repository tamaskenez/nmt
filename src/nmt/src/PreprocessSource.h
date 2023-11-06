#pragma once

#include "data.h"

std::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv);

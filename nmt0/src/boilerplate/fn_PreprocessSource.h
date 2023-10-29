#pragma once

#include "sc_PreprocessedSource.h"

std::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv);

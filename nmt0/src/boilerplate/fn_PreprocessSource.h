#pragma once

#include "sc_PreprocessedSource.h"

#include <expected>

#include <string_view>

std::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv);

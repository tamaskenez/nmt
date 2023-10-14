#pragma once

#include "sc_PreprocessedSource.h"

#include <nonstd/expected.hpp>

#include <string_view>

nonstd::expected<PreprocessedSource, std::string> PreprocessSource(std::string_view sv);
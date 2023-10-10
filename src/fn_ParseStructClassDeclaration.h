#pragma once

#include <nonstd/expected.hpp>

#include <string>
#include <string_view>
#include <utility>

nonstd::expected<std::pair<std::string, std::string>, std::string> ParseStructClassDeclaration(
    std::string_view sv);

#pragma once

#include <expected>

#include <string>
#include <string_view>
#include <utility>

std::expected<std::pair<std::string, std::string>, std::string> ParseOpaqueEnumDeclaration(
    std::string_view sv);

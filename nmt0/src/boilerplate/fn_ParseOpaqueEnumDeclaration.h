#pragma once

std::expected<std::pair<std::string, std::string>, std::string> ParseOpaqueEnumDeclaration(
    std::string_view sv);

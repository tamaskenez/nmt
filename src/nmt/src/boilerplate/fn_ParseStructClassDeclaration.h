#pragma once

std::expected<std::pair<std::string, std::string>, std::string> ParseStructClassDeclaration(
    std::string_view sv);
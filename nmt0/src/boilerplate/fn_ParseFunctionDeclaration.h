#pragma once

std::expected<std::pair<std::string, std::string>, std::string> ParseFunctionDeclaration(
    std::string_view sv);

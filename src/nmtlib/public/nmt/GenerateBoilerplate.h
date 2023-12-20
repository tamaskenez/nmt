#pragma once

#include <expected>
#include <string>
#include <variant>

struct Project;

std::expected<std::monostate, std::vector<std::string>> GenerateBoilerplate(const Project& project);

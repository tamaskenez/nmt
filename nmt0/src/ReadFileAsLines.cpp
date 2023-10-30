#include "pch.h"

#include "ReadFileAsLines.h"

std::optional<std::vector<std::string>> ReadFileAsLines(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f.is_open()) {
        return std::nullopt;
    }
    std::vector<std::string> result;
    std::string line;
    while (std::getline(f, line)) {
        result.push_back(line);  // Copy.
    }
    return result;
}

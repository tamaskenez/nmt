#include "pch.h"

#include "ReadFile.h"

std::optional<std::string> ReadFile(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f.is_open()) {
        return std::nullopt;
    }
    std::string result;
    std::string line;
    while (std::getline(f, line)) {
        result += line;
        result += '\n';
    }
    return result;
}

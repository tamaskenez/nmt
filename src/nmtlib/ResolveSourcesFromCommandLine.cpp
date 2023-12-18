#include "nmt/ResolveSourcesFromCommandLine.h"

#include "nmt/constants.h"
#include "util/ReadFileAsLines.h"

namespace fs = std::filesystem;

std::expected<ResolveSourcesFromCommandLineResult, std::string> ResolveSourcesFromCommandLine(
    std::vector<fs::path> sourcesFromCommandLine, bool verbose) {
    std::set<fs::path> pathsDone;
    std::vector<fs::path> sources;
    std::vector<std::string> messages;

    while (!sourcesFromCommandLine.empty()) {
        auto f = std::move(sourcesFromCommandLine.back());
        sourcesFromCommandLine.pop_back();
        std::error_code ec;
        auto cf = fs::canonical(f, ec);
        if (ec) {
            return std::unexpected(fmt::format("File doesn't exist: `{}`", f));
        }
        if (pathsDone.contains(cf)) {
            // Duplicate.
            continue;
        }
        pathsDone.insert(cf);
        auto ext = cf.extension();
        if (k_validSourceExtensions.contains(ext)) {
            sources.push_back(cf);
        } else if (k_fileListExtensions.contains(ext)) {
            auto pathList = ReadFileAsLines(f);
            if (!pathList) {
                return std::unexpected(fmt::format("Can't read file list from `{}`.", f));
            }
            sourcesFromCommandLine.reserve(sourcesFromCommandLine.size() + pathList->size());
            std::optional<fs::path> base;
            if (f.has_parent_path()) {
                base = f.parent_path();
            }
            for (auto& p : *pathList) {
                if (!p.empty()) {
                    if (base) {
                        p = *base / p;
                    }
                    sourcesFromCommandLine.push_back(std::move(p));
                }
            }
        } else {
            // Ignore this file.
            if (verbose) {
                messages.push_back(fmt::format("Ignoring file with extension `{}`: {}", ext, f));
            }
        }
    }

    std::sort(sources.begin(), sources.end());
    return ResolveSourcesFromCommandLineResult{std::move(sources), std::move(messages)};
}

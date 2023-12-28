#include "nmt/GlobSourceFiles.h"

#include "nmt/constants.h"
#include "util/ReadFileAsLines.h"

namespace fs = std::filesystem;

namespace {
std::string_view to_string_view(fs::file_type x) {
    switch (x) {
#define CASE(X)            \
    case fs::file_type::X: \
        return #X;
        CASE(none)
        CASE(not_found)
        CASE(regular)
        CASE(directory)
        CASE(symlink)
        CASE(block)
        CASE(character)
        CASE(fifo)
        CASE(socket)
        CASE(unknown)
#undef CASE
    }
}
}  // namespace

std::expected<GlobSourceFilesResult, std::vector<std::string>> GlobSourceFiles(
    const std::filesystem::path& sourceDir, bool verbose) {
    std::vector<fs::path> sources;
    std::vector<std::string> messages;

    std::error_code ec;
    auto b = fs::recursive_directory_iterator(sourceDir, ec);
    if (ec) {
        return std::unexpected(make_vector(fmt::format(
            "Failed to collect source files from `{}`, reason: {}", sourceDir, ec.message())));
    }
    std::vector<std::string> errors;
    for (auto it = b; it != fs::recursive_directory_iterator(); ++it) {
        auto status = it->symlink_status(ec);
        if (ec) {
            errors.push_back(
                fmt::format("Can't get status of file `{}`, reason: {}", it->path(), ec.message()));
            continue;
        }
        switch (status.type()) {
            using enum fs::file_type;
            case not_found:
                errors.push_back(
                    fmt::format("File `{}` has a type of `not_found` and that's strange, what "
                                "should we do with that? For now it's an error",
                                it->path()));
                break;
            case regular: {
                auto ext = it->path().extension();
                if (k_validSourceExtensions.contains(ext)) {
                    sources.push_back(it->path());
                } else if (verbose) {
                    messages.push_back(
                        fmt::format("Ignoring file with extension `{}`: {}", ext, it->path()));
                }
            } break;
            case directory:
                break;
                ;
            case none:
            case symlink:
            case block:
            case character:
            case fifo:
            case socket:
            case unknown:
                errors.push_back(fmt::format(
                    "File `{}` has a type of {} and it's not yet decided what to do with this type",
                    it->path(),
                    to_string_view(status.type())));
                break;
        }
    }
    if (!errors.empty()) {
        return std::unexpected(std::move(errors));
    }
    /*
    while (!sourcesFromCommandLine.empty()) {
        auto f = std::move(sourcesFromCommandLine.back());
        sourcesFromCommandLine.pop_back();
        std::error_code ec;
        auto cf = fs::canonical(f, ec);
        if (ec) {
            return std::unexpected(
                fmt::format("Error processing source file `{}`: {}", f, ec.message()));
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
        }
    }

     */
    std::sort(sources.begin(), sources.end());
    return GlobSourceFilesResult{std::move(sources), std::move(messages)};
}

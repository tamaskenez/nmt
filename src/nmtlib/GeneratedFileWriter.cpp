#include "GeneratedFileWriter.h"

#include "ReadFile.h"
#include "WriteFile.h"

namespace fs = std::filesystem;

GeneratedFileWriter::GeneratedFileWriter(fs::path outputDir_)
    : outputDir(std::move(outputDir_)) {
    std::error_code ec;
    fs::create_directories(outputDir, ec);
    LOG_IF(QFATAL, ec) << fmt::format("Can't create output directory `{}`.", outputDir);

    // Enumerate current files in the output directory.
    auto dit = fs::recursive_directory_iterator(outputDir, fs::directory_options::none, ec);
    LOG_IF(QFATAL, ec) << fmt::format("Can't get listing of output directory `{}`.", outputDir);
    for (auto const& de : dit) {
        auto d = de.is_directory(ec);
        LOG_IF(FATAL, ec) << fmt::format("Can't query if directory entry is a directory: {}",
                                         de.path());
        if (d) {
            remainingExistingDirs.insert(de.path());
        } else {
            remainingExistingFiles.insert(de.path());
        }
    }
}
GeneratedFileWriter::~GeneratedFileWriter() {
    assert(remainingExistingFiles.empty() && remainingExistingDirs.empty());
    RemoveRemainingExistingFilesAndDirs();
}
void GeneratedFileWriter::Write(const fs::path& relPath, std::string_view content) {
    auto relDir = relPath;
    bool firstParent = true;
    for (; relDir.has_parent_path();) {
        relDir = relDir.parent_path();
        auto absDir = outputDir / relDir;
        if (remainingExistingDirs.erase(absDir) == 0 && firstParent) {
            std::error_code ec;
            [[maybe_unused]] bool created = fs::create_directories(absDir, ec);
            LOG_IF(FATAL, ec) << fmt::format("Couldn't create directories: {}", absDir);
        }
        firstParent = false;
    }
    bool create_directories(const std::filesystem::path& p, std::error_code& ec);

    auto path = outputDir / relPath;
    remainingExistingFiles.erase(path);
    currentFiles.push_back(path);
    auto existingContent = ReadFile(path);
    if (existingContent != content) {
        LOG_IF(FATAL, !WriteFile(path, content)) << fmt::format("Couldn't write {}.", path);
    }  // Else no change, no need to write.
}
void GeneratedFileWriter::RemoveRemainingExistingFilesAndDirs() {
    std::error_code ec;
    for (auto& f : remainingExistingFiles) {
        fs::remove(f, ec);  // Ignore if couldn't remove it.
    }
    remainingExistingFiles.clear();
    for (auto& d : remainingExistingDirs) {
        fs::remove_all(d, ec);
    }
    remainingExistingDirs.clear();  // Ignore if couldn't remove it.
}

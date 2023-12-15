#pragma once

#include "nmt/base_types.h"

struct GeneratedFileWriter {
    explicit GeneratedFileWriter(std::filesystem::path outputDir_);
    ~GeneratedFileWriter();
    void Write(const std::filesystem::path& relPath, std::string_view content);
    void RemoveRemainingExistingFilesAndDirs();

    // All paths should be absolute.
    std::filesystem::path outputDir;
    flat_hash_set<std::filesystem::path, path_hash> remainingExistingFiles;
    std::vector<std::filesystem::path> currentFiles;
    flat_hash_set<std::filesystem::path, path_hash> remainingExistingDirs;
};

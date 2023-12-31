#pragma once

#include "nmt/base_types.h"

struct GeneratedFileWriter {
    explicit GeneratedFileWriter(std::filesystem::path outputDir_);
    GeneratedFileWriter(const GeneratedFileWriter&) = delete;
    GeneratedFileWriter(GeneratedFileWriter&& y)
        : outputDir(std::move(y.outputDir))
        , remainingExistingFiles(std::move(y.remainingExistingFiles))
        , currentFiles(std::move(y.currentFiles))
        , remainingExistingDirs(std::move(y.remainingExistingDirs)) {
        y.remainingExistingFiles.clear();
        y.remainingExistingDirs.clear();
    }
    ~GeneratedFileWriter();
    void Write(const std::filesystem::path& relPath, std::string_view content);
    void RemoveRemainingExistingFilesAndDirs();

    // All paths should be absolute.
    std::filesystem::path outputDir;
    flat_hash_set<std::filesystem::path, path_hash> remainingExistingFiles;
    std::vector<std::filesystem::path> currentFiles;
    flat_hash_set<std::filesystem::path, path_hash> remainingExistingDirs;
};

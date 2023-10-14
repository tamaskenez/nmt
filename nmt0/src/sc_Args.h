// needs: <filesystem>, <vector>
struct Args {
    std::vector<std::filesystem::path> sources, sourcesFiles;
    std::filesystem::path outputDir;
};

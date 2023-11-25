// needs: <filesystem>, <vector>
struct ProgramOptions {
    bool verbose = false;
    std::vector<std::filesystem::path> sources;
    std::filesystem::path outputDir;
};

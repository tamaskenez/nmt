// needs: <filesystem>, <vector>
struct Args {
    bool verbose = false;
    std::vector<std::filesystem::path> sources;
    std::filesystem::path outputDir;
};

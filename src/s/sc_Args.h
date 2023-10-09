// needs: <filesystem>, <vector>
struct Args {
    std::vector<std::filesystem::path> sourceFiles;
    std::filesystem::path outputDir;
};

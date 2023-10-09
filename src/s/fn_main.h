int main(int argc, char* argv[])
// needs: <cstdlib>, <glog/logging.h>, <fmt/std.h>, ReadArgs, EntityKindOfStem, ReadFileAsLines,
// needs: PreprocessSource
{
    namespace fs = std::filesystem;

    google::InitGoogleLogging(argv[0]);
    // Read args.
    auto args = ReadArgs(argc, argv);
    fmt::print("-s: {}\n", fmt::join(args.sourceFiles, ", "));
    fmt::print("-o: {}\n", args.outputDir);
    int result = EXIT_SUCCESS;
    for (auto& sf : args.sourceFiles) {
        // Find kind.
        auto kind = EntityKindOfStem(sf.stem().string());
        LOG_IF(FATAL, !kind) << fmt::format(
            "Source file name {} is not prefixed by Entitykind ({})", sf.filename(), sf);
        auto maybeSource = ReadFile(sf);
        LOG_IF(FATAL, !maybeSource) << fmt::format("Can't open file for reading: {}", sf);
        auto& source = *maybeSource;

        auto pps = PreprocessSource(source);
        LOG(FATAL);
        // ...
    }
    return result;
}

// needs: Args
Args ReadArgs(int argc, char* argv[])
// needs: <string_view>, <filesystem>, <optional>, <glog/logging.h>, <utility>, <vector>
{
    using path = std::filesystem::path;
    std::vector<path> arg_s;
    std::optional<path> arg_o;
    for (int i = 1; i < argc; ++i) {
        std::string_view ai = argv[i];
        auto nextArg = [&i, argv, argc](std::string_view argName) -> std::string_view {
            LOG_IF(FATAL, i + 1 >= argc) << "Missing argument after " << argName;
            ++i;
            return argv[i];
        };
        if (ai == "--source-files") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                arg_s.emplace_back(reinterpret_u8string_view(argv[++i]));
            }
        } else if (ai == "--output-dir") {
            LOG_IF(FATAL, arg_o) << "--output-dir specified multiple times";
            arg_o = path(reinterpret_u8string_view(nextArg("--output-dir")));
        } else {
            LOG(FATAL) << "Invalid argument: " << ai;
        }
    }
    LOG_IF(FATAL, arg_s.empty()) << "No source files.";
    LOG_IF(FATAL, !arg_o) << "--output-dir not specified.";
    return Args{.sourceFiles = std::move(arg_s), .outputDir = std::move(*arg_o)};
}

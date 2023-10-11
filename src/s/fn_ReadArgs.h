// needs: Args
Args ReadArgs(int argc, char* argv[])
// needs: <string_view>, <filesystem>, <optional>, <glog/logging.h>, <utility>, <vector>
{
    using path = std::filesystem::path;
    std::vector<path> arg_s, arg_sf;
    std::optional<path> arg_o;
    for (int i = 1; i < argc; ++i) {
        std::string_view ai = argv[i];
        auto nextArg = [&i, argv, argc](std::string_view argName) -> std::string_view {
            LOG_IF(FATAL, i + 1 >= argc) << "Missing argument after " << argName;
            ++i;
            return argv[i];
        };
        if (ai == "--sources") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                arg_s.emplace_back(reinterpret_u8string_view(argv[++i]));
            }
        } else if (ai == "--output-dir") {
            LOG_IF(FATAL, arg_o) << "--output-dir specified multiple times";
            arg_o = path(reinterpret_u8string_view(nextArg("--output-dir")));
        } else if (ai == "--sources-files") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                arg_sf.emplace_back(reinterpret_u8string_view(argv[++i]));
            }
        } else {
            LOG(FATAL) << "Invalid argument: " << ai;
        }
    }
    LOG_IF(FATAL, !arg_o) << "--output-dir not specified.";
    return Args{.sources = std::move(arg_s),
                .sourcesFiles = std::move(arg_sf),
                .outputDir = std::move(*arg_o)};
}

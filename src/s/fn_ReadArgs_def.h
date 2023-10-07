Args ReadArgs(int argc, char* argv[]) {
    using path = std::filesystem::path;
    std::optional<path> arg_i, arg_o;
    for (int i = 1; i < argc; ++i) {
        std::string_view ai = argv[i];
        auto nextArg = [&i, argv, argc](std::string_view argName) -> std::string_view {
            LOG_IF(FATAL, i + 1 >= argc) << "Missing argument after " << argName;
            ++i;
            return argv[i];
        };
        if (ai == "-i") {
            LOG_IF(FATAL, arg_i) << "-i specified multiple times";
            arg_i = path(reinterpret_u8string_view(nextArg("-i")));
        } else if (ai == "-o") {
            LOG_IF(FATAL, arg_o) << "-o specified multiple times";
            arg_o = path(reinterpret_u8string_view(nextArg("-j")));
        } else {
            LOG(FATAL) << "Invalid argument: " << ai;
        }
    }
    LOG_IF(FATAL, !arg_i) << "-i not specified.";
    LOG_IF(FATAL, !arg_o) << "-o not specified.";
    return Args{.i = std::move(*arg_i), .o = std::move(*arg_o)};
}

#include "nmt/ProgramOptions.h"

#include "nmt/constants.h"

std::expected<ProgramOptions, int> ParseProgramOptions(int argc, char* argv[]) {
    CLI::App app("C++ boilerplate generator", "NMT");

    ProgramOptions args;

    app.add_option(
        "-s,--sources",
        args.sources,
        fmt::format("List of source files ({}) or files containing a list of source files ({}, one "
                    "path on each line) or any other files (they'll be ignored)",
                    fmt::join(k_validSourceExtensions, ", "),
                    fmt::join(k_fileListExtensions, ", ")));
    app.add_option("-o,--output-dir",
                   args.outputDir,
                   "Output directory, will be created or content erased/updated, as needed")
        ->required();
    app.add_flag("-v,--verbose", args.verbose, "Print more diagnostics");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return std::unexpected(app.exit(e));
    }

    return args;
}

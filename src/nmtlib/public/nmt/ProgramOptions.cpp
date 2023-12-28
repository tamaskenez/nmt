#include "nmt/ProgramOptions.h"

#include "nmt/constants.h"

std::expected<ProgramOptions, int> ParseProgramOptions(int argc, char* argv[]) {
    CLI::App app("C++ boilerplate generator", "NMT");

    ProgramOptions args;

    app.add_option("-s,--source-dir",
                   args.sourceDir,
                   fmt::format("Directory containing the source files for target. The directory "
                               "will be recursively globbed for source files ({})",
                               fmt::join(k_validSourceExtensions, ", ")))
        ->required();
    app.add_option("-t,--target",
                   args.target,
                   "Name of the target (library). Public header files will be generated into "
                   "`<output-dir>/<target>/`")
        ->required();
    app.add_option(
           "-o,--output-dir",
           args.outputDir,
           fmt::format("Output directory, will be created or content erased/updated, as needed. "
                       "List of generated files will be written to `<output-dir>/{}`",
                       k_fileListFilename))
        ->required();
    app.add_flag("-v,--verbose", args.verbose, "Print more diagnostics");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return std::unexpected(app.exit(e));
    }

    return args;
}

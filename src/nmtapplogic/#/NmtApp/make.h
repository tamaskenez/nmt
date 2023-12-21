// #memfn
STATIC std::expected<std::unique_ptr<NmtApp>, std::vector<std::string>> NmtApp::make(int argc,
                                                                                     char* argv[]) {
    auto programOptionsOr = ParseProgramOptions(argc, argv);
    if (!programOptionsOr) {
        return std::unexpected(make_vector(fmt::format(
            "Invalid command line arguments, CLI11 error code: {}", programOptionsOr.error())));
    }
    auto& programOptions = *programOptionsOr;
    TRY_ASSIGN_OR_RETURN_VALUE(
        sources,
        ResolveSourcesFromCommandLine(programOptions.sources, programOptions.verbose),
        std::unexpected(make_vector(std::move(UNEXPECTED_ERROR))));
    for (auto& m : sources.messages) {
        fmt::print("{}\n", m);
    }

    Project project(programOptions.outputDir);
    std::vector<std::string> errors;
    for (auto& s : sources.resolvedSources) {
        auto r = project.entities.addSource(s);
        if (!r) {
            errors.push_back(std::move(r.error()));
        }
    }
    if (!errors.empty()) {
        return std::unexpected(std::move(errors));
    }
    /*
    for (auto id : project.entities.dirtySources()) {
        auto [newErrors, newVerboseMessages] = ProcessSourceAndUpdateProject(project, id,
    args.verbose); append_range(errors, std::move(newErrors)); if (args.verbose) {
            append_range(verboseMessages, std::move(newVerboseMessages));
        }
    }
    if (args.verbose) {
        for (auto& m : verboseMessages) {
            fmt::print("Note: {}\n", m);
        }
    }
    for (auto& m : errors) {
        fmt::print("ERROR: {}\n", m);
    }
    if (!errors.empty()) {
        return EXIT_FAILURE;
    }
*/
    return std::make_unique<NmtAppImpl>(std::move(project));
}
// #needs: <memory>, <expected>, <vector>, <string>
// #defneeds: NmtAppImpl,
// "nmt/ProgramOptions.h","util/error.h","nmt/Project.h","nmt/ResolveSourcesFromCommandLine.h"

// #visibility: public

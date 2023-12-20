// #memfn
STATIC std::expected<std::unique_ptr<NmtApp>, std::string> NmtApp::make(int argc, char* argv[]) {
    auto programOptionsOr = ParseProgramOptions(argc, argv);
    if (!programOptionsOr) {
        return std::unexpected(fmt::format("Invalid command line arguments, CLI11 error code: {}",
                                           programOptionsOr.error()));
    }
    auto& programOptions = *programOptionsOr;
    TRY_ASSIGN(sources,
               ResolveSourcesFromCommandLine(programOptions.sources, programOptions.verbose));
    for (auto& m : sources.messages) {
        fmt::print("{}\n", m);
    }

    Project project(programOptions.outputDir);
    TRY_ASSIGN(msgs, project.AddAndProcessSources(sources.resolvedSources, programOptions.verbose));
    for (auto& m : msgs) {
        fmt::print("{}\n", m);
    }

    return std::make_unique<NmtAppImpl>(std::move(project));
}
// #needs: <memory>, <expected>
// #defneeds: NmtAppImpl, "nmt/ProgramOptions.h",
// "util/error.h","nmt/Project.h","nmt/ResolveSourcesFromCommandLine.h" #visibility: public

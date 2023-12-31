// #memfn
STATIC std::expected<std::unique_ptr<NmtApp>, std::vector<std::string>> NmtApp::make(int argc,
                                                                                     char* argv[]) {
    TRY_ASSIGN_OR_RETURN_VALUE(
        programOptions,
        ParseProgramOptions(argc, argv),
        std::unexpected(make_vector(fmt::format(
            "Invalid command line arguments, CLI11 error code: {}", UNEXPECTED_ERROR))));
    TRY_ASSIGN(sources, GlobSourceFiles(programOptions.sourceDir, programOptions.verbose));

    Project project;
    TRY_ASSIGN_OR_RETURN_VALUE(
        targetId,
        project.addTarget(
            programOptions.target, programOptions.sourceDir, programOptions.outputDir),
        std::unexpected(make_vector(UNEXPECTED_ERROR)));

    auto& target = project.targets.at(targetId);
    std::vector<std::string> errors;
    for (auto& s : sources.sources) {
        auto r = project.entities.addSource(targetId, target.sourceDir, s);
        if (!r) {
            errors.push_back(std::move(r.error()));
        }
    }
    if (!errors.empty()) {
        return std::unexpected(std::move(errors));
    }

    return std::make_unique<NmtAppImpl>(std::move(project));
}
// #needs: <memory>, <expected>, <vector>, <string>
// #defneeds: NmtAppImpl,
// "nmt/ProgramOptions.h","util/error.h","nmt/Project.h", "nmt/GlobSourceFiles.h"

// #visibility: public

#include "nmt/Project.h"

#include "nmt/ProcessSource.h"

Project::Project(std::filesystem::path outputDir)
    : outputDir(std::move(outputDir)) {}

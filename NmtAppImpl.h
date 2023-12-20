// #memfn
EXPLICIT NmtAppImpl::NmtAppImpl(Project project)
    : userState(std::move(project)) {}
// #needs: "nmt/Project.h"

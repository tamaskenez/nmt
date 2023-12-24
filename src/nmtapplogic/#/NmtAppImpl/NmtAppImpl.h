// #memfn
EXPLICIT NmtAppImpl::NmtAppImpl(Project project)
    : userState(std::make_unique<UserState>()) {
    userState->project = std::move(project);
}
// #needs: "nmt/Project.h"
// #defneeds: <utility>, <memory>, "appcommon/UserState.h"

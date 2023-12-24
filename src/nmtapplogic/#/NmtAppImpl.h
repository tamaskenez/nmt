// #struct
struct NmtAppImpl : NmtApp {
    NmtAppImpl() = delete;
    std::unique_ptr<UserState> userState;
#include NMT_MEMBER_DECLARATIONS
};
// #needs: NmtApp, <memory>, struct UserState

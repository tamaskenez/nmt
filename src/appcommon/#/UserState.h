// #struct
struct UserState {
    std::optional<Project> project;

#include NMT_MEMBER_DECLARATIONS
};
// #needs: "nmt/Project.h", <optional>
// #visibility: public

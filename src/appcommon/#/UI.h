// #class
class UI {
   public:
    virtual ~UI() = default;

    virtual int exec(const UserState* userState) = 0;
};
// #visibility: public
// #needs: UserState*

class ProjectTreeView {
   public:
    static constexpr int64_t k_rootId = 0;
    virtual ~ProjectTreeView() = default;
    // virtual int64_t addItem(int64_t parentId,
};

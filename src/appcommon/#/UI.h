// #class
class UI {
   public:
    virtual ~UI() = default;

    virtual int exec(const UserState* userState) = 0;
};
// #visibility: public
// #needs: UserState*

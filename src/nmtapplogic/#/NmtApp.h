// #class
class NmtApp {
   public:
    virtual ~NmtApp() = default;

    virtual int run(std::unique_ptr<UI> ui) = 0;

#include NMT_MEMBER_DECLARATIONS
};

// #visibility: public
// #needs: <memory>, class UI
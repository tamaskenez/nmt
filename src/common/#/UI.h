// #class
class UI {
   public:
    virtual ~UI() = default;

    virtual int exec() = 0;
};
// #visibility:public
#pragma once

#include <memory>

class UI;

class NmtApp {
   public:
    static std::unique_ptr<NmtApp> make(int argc, char* argv[]);
    virtual ~NmtApp() = default;

    virtual int run(std::unique_ptr<UI> ui) = 0;
};

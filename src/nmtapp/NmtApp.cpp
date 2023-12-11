#include "NmtApp.h"

#include "UI.h"

struct NmtAppImpl : NmtApp {
    int run(std::unique_ptr<UI> ui) override {
        return ui->exec();
    }
};

std::unique_ptr<NmtApp> NmtApp::make(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return std::make_unique<NmtAppImpl>();
}

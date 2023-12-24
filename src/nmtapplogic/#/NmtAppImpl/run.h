// #memfn
int NmtAppImpl::run(std::unique_ptr<UI> ui) OVERRIDE {
    return ui->exec(userState.get());
}
// #needs: <memory>, class UI
// #defneeds: "appcommon/UI.h"

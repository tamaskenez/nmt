// #memfn
int NmtAppImpl::run(std::unique_ptr<UI> ui) OVERRIDE {
    return ui->exec();
}
// #needs: <memory>, "appcommon/UI.h"
// #defneeds: "appcommon/UI.h"

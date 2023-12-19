// #memfn
int NmtAppImpl::run(std::unique_ptr<UI> ui) OVERRIDE {
    return ui->exec();
}
// #needs: <memory>, "UI.h"
// #defneeds: "UI.h"

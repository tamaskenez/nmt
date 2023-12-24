#include "QtUI.h"

#include "MainWindow.h"

#include <QApplication>

struct QtUI : UI {
    QApplication _application;
    const UserState* userState;

    QtUI(int& argc, char* argv[])
        : _application(argc, argv) {}
    int exec(const UserState* userStateArg) override {
        userState = userStateArg;
        MainWindow mw;
        mw.show();
        return _application.exec();
    }
};

std::unique_ptr<UI> QtUI_make(int& argc, char* argv[]) {
    return std::make_unique<QtUI>(argc, argv);
}

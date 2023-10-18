#include "QtUI.hpp"

#include "MainWindow.hpp"

#include <QApplication>

struct QtUI : UI {
    QApplication _application;

    QtUI(int& argc, char* argv[])
        : _application(argc, argv) {}
    int exec() override {
        MainWindow mw;
        mw.show();
        return _application.exec();
    }
};

std::unique_ptr<UI> QtUI_make(int& argc, char* argv[]) {
    return std::make_unique<QtUI>(argc, argv);
}

#include "MainWindow.h"

#include <QGuiApplication>
#include <QScreen>

MainWindow::MainWindow() {
    auto* cw = new QWidget;
    setCentralWidget(cw);

    setWindowTitle(tr("NMT"));

    auto as = screen()->availableSize();
    resize(as.width() / 2, as.height() / 2);
    setMinimumSize(160, 160);
}

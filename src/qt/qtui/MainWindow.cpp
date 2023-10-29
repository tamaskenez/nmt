#include "MainWindow.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTextEdit>

MainWindow::MainWindow() {
    editor = new QTextEdit;

    setCentralWidget(editor);

    setWindowTitle(tr("NMT"));

    auto as = screen()->availableSize();
    resize(as.width() / 2, as.height() / 2);
    setMinimumSize(160, 160);
}

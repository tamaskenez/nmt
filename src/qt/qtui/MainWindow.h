#pragma once

#include <QMainWindow>

class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow();

   private:
    QTextEdit* editor;
};

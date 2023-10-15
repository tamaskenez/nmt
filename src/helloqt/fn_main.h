// needs: <QApplication>, <QMainWindow>
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QMainWindow win;
    win.resize(320, 240);
    win.setVisible(true);

    return app.exec();
}

#include "QtUI.h"

#include "ProjectTreeItemModel.h"
#include "appcommon/UserState.h"

struct MainWindow : public QMainWindow {
    QTreeView* projectTree;
    MainWindow() {
        setWindowTitle(tr("NMT"));

        auto* hSplitter = new QSplitter(Qt::Horizontal);
        setCentralWidget(hSplitter);

        projectTree = new QTreeView;

        hSplitter->addWidget(projectTree);
        hSplitter->addWidget(new QWidget);

        auto as = screen()->availableSize();
        resize(as.width() / 2, as.height() / 2);
        setMinimumSize(160, 160);
    }
};

struct QtUI : UI {
    QApplication _application;

    QtUI(int& argc, char* argv[])
        : _application(argc, argv) {}
    int exec(const UserState* userState) override {
        auto sidePanelModel = makeProjectTreeItemModel(userState->project);

        MainWindow mainWindow;
        mainWindow.projectTree->setModel(sidePanelModel.get());

        mainWindow.show();
        return _application.exec();
    }
};

std::unique_ptr<UI> makeQtUI(int& argc, char* argv[]) {
    return std::make_unique<QtUI>(argc, argv);
}

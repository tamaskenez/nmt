#include "MainWindow.h"

class SourcesTreeItemModel : public QAbstractItemModel {
   public:
    explicit SourcesTreeItemModel(QObject* parent = nullptr)
        : QAbstractItemModel(parent) {}

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid()) {
            return QVariant();
        }

        if (role != Qt::DisplayRole) {
            return QVariant();
        }
        if (index == this->index(0, 0) || index == this->index(1, 0)) {
            return QString("item (%0, %1), role: %2")
                .arg(index.row())
                .arg(index.column())
                .arg(role);
        }
        assert(false);
        return {};
    }
    Qt::ItemFlags flags(const QModelIndex& index) const override {
        if (!index.isValid()) {
            return Qt::NoItemFlags;
        }
        return QAbstractItemModel::flags(index);
    }
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return QString("header %0 %1 %2").arg(section).arg(orientation).arg(role);
        }
        return QVariant();
    }
    QModelIndex index(int row,
                      int column,
                      const QModelIndex& parent = QModelIndex()) const override {
        if (!hasIndex(row, column, parent)) {
            return QModelIndex();
        }
        if (parent == QModelIndex() && (row == 0 || row == 1) && column == 0) {
            return createIndex(row, 0, quintptr(0));
        }
        assert(false);
        return QModelIndex();
    }
    QModelIndex parent(const QModelIndex& child) const override {
        if (!child.isValid()) {
            return QModelIndex();
        }
        if (child == index(0, 0) || child == index(1, 0)) {
            return QModelIndex();
        }
        assert(false);
        return QModelIndex();
    }
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        if (parent == QModelIndex()) {
            return 2;
        }
        return 0;
    }
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        if (parent == QModelIndex()) {
            return 1;
        }
        return 0;
    }
};

class SourcesTreeItemModel2 : public QAbstractItemModel {
   public:
    QStandardItemModel* q;
    explicit SourcesTreeItemModel2(QObject* parent = nullptr)
        : QAbstractItemModel(parent) {
        q = new QStandardItemModel;
        QStandardItem* parentItem = q->invisibleRootItem();
        for (int i = 0; i < 2; ++i) {
            auto* item = new QStandardItem(QString("item %0").arg(i));
            parentItem->appendRow(item);
            // parentItem = item;
        }
    }

    QModelIndex index(int row,
                      int column,
                      const QModelIndex& parent = QModelIndex()) const override {
        auto r = q->index(row, column, parent);
        return r;
    }
    QModelIndex parent(const QModelIndex& child) const override {
        auto r = q->parent(child);
        return r;
    }
    QModelIndex sibling(int row, int column, const QModelIndex& idx) const override {
        auto r = q->sibling(row, column, idx);
        return r;
    }
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        auto r = q->rowCount(parent);
        return r;
    }
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        auto r = q->columnCount(parent);
        return r;
    }
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override {
        auto r = q->hasChildren(parent);
        return r;
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        auto r = q->data(index, role);
        return r;
    }
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override {
        auto r = q->headerData(section, orientation, role);
        return r;
    }
    QMap<int, QVariant> itemData(const QModelIndex& index) const override {
        auto r = q->itemData(index);
        return r;
    }
    QStringList mimeTypes() const override {
        auto r = q->mimeTypes();
        return r;
    }
    QMimeData* mimeData(const QModelIndexList& indexes) const override {
        auto r = q->mimeData(indexes);
        return r;
    }
    /*
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column, const QModelIndex &parent) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                              int row, int column, const QModelIndex &parent);
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::DropActions supportedDragActions() const;
*/

    void fetchMore(const QModelIndex& parent) override {
        q->fetchMore(parent);
    }
    bool canFetchMore(const QModelIndex& parent) const override {
        auto r = q->canFetchMore(parent);
        return r;
    }
    Qt::ItemFlags flags(const QModelIndex& index) const override {
        auto r = q->flags(index);
        return r;
    }
    // Q_INVOKABLE Q_REVISION(6, 4) virtual void sort(int column, Qt::SortOrder order =
    // Qt::AscendingOrder);
    QModelIndex buddy(const QModelIndex& index) const override {
        auto r = q->buddy(index);
        return r;
    }
    /*
    Q_INVOKABLE virtual QModelIndexList match(const QModelIndex &start, int role,
                                              const QVariant &value, int hits = 1,
                                              Qt::MatchFlags flags =
                                              Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap))
    const;
     */
    QSize span(const QModelIndex& index) const override {
        auto r = q->span(index);
        return r;
    }

    QHash<int, QByteArray> roleNames() const override {
        auto r = q->roleNames();
        return r;
    }
};

MainWindow::MainWindow() {
    setWindowTitle(tr("NMT"));

    auto* hSplitter = new QSplitter(Qt::Horizontal);
    setCentralWidget(hSplitter);

    [[maybe_unused]] auto* model = new SourcesTreeItemModel;
    [[maybe_unused]] auto* model2 = new QStandardItemModel;
    [[maybe_unused]] auto* model3 = new SourcesTreeItemModel2;

    QStandardItem* parentItem = model2->invisibleRootItem();
    for (int i = 0; i < 2; ++i) {
        QStandardItem* item = new QStandardItem(QString("item %0").arg(i));
        parentItem->appendRow(item);
        // parentItem = item;
    }
    [[maybe_unused]] auto x1 = model2->columnCount();
    [[maybe_unused]] auto x2 = model2->rowCount();
    [[maybe_unused]] auto x3 = model2->rowCount(model2->index(0, 0));
    //    [[maybe_unused]] auto x2 =model2->index(0,0);

    QTreeView* tree = new QTreeView;
    tree->setModel(model);

    hSplitter->addWidget(tree);
    hSplitter->addWidget(new QWidget);

    auto as = screen()->availableSize();
    resize(as.width() / 2, as.height() / 2);
    setMinimumSize(160, 160);
}

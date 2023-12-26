#include "ProjectTreeItemModel.h"

#include "nmt/Project.h"
#include "nmt/base_types.h"

/*
struct Subdir {
    std::vector<SubdirId> subdirs;
    std::vector<EntityId> entities;
};

map<SubdirId, Subdir> subdirs;
map<fs::path, SubdirId>

{
    Subdirid, parentsubdirid
    entityid
    memfnentityid
}

Subdir {
    Subdir*
    (Header|Class|Struct|Fn)*
}

Class, Struct {
    MemFn*
}
*/

struct ProjectTreeItemModel : public QAbstractItemModel {
    explicit ProjectTreeItemModel(const std::optional<Project>& project, QObject* parent)
        : QAbstractItemModel(parent)
        , project(project) {}

    const std::optional<Project>& project;

    /*
    struct Subdir {
        std::map<std::string, int64_t> subdirs;
    };
    flat_hash_map<int64_t, Subdir> idToSubdir;

    void f() {
        constexpr int64_t k_rootSubdirId = 0;
        for (auto& [k, v] : project->entities.items) {
            auto it = v.sourcePath.begin();
            CHECK(it != v.sourcePath.end());
            auto subdirId = getOrMakeSubdir(path_to_string(*it), k_rootSubdirId);
            auto s = ;
            roots.insert(std::make_pair(s,
            for(auto&pc:v.sourcePath){
                if(
            }
        }
    }
*/
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.column() != 0) {
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

std::unique_ptr<QAbstractItemModel> makeProjectTreeItemModel(
    const std::optional<Project>& project) {
    return std::make_unique<ProjectTreeItemModel>(project, nullptr);
}
QAbstractItemModel* makeProjectTreeItemModel(const std::optional<Project>& project,
                                             QObject* parent) {
    return new ProjectTreeItemModel(project, parent);
}

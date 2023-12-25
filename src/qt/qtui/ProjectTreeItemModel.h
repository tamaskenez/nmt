#pragma once

class QAbstractItemModel;
struct Project;
class QObject;

std::unique_ptr<QAbstractItemModel> makeProjectTreeItemModel(const std::optional<Project>& project);
QAbstractItemModel* makeProjectTreeItemModel(const std::optional<Project>& project,
                                             QObject* parent);

#pragma once

#include "frameinfo.h"
#include <QAbstractItemModel>
#include <QFile>
using namespace std;

/**
 * @class ModelFrameList
 * @brief 继承QAbstractTableModel，和tableView绑定
 */
class ModelFrameList : public QAbstractTableModel {
    Q_OBJECT
  public:
    // view相关
    explicit ModelFrameList(const vector<shared_ptr<FLVFrame>>& list, QObject* parent = nullptr)
        : QAbstractTableModel(parent), m_frameList(list) {
    }
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 数据相关
    vector<shared_ptr<FLVFrame>> getFrameList() {
        return m_frameList;
    }
    int readFromFile(QFile& path);
    static const int column_size = 5;

    // 添加删除方法
    bool removeRow(int row, const QModelIndex& parent = QModelIndex()) {
        beginRemoveRows(parent, row, row);
        m_frameList.erase(m_frameList.begin() + row);
        endRemoveRows();
        return true;
    }

  private:
    vector<shared_ptr<FLVFrame>> m_frameList;
};

/**
 * @class ModelFrameInfoTree
 * @brief 继承QAbstractItemModel，和treeView绑定
 */
class ModelFrameInfoTree : public QAbstractItemModel {
    Q_OBJECT
  public:
    explicit ModelFrameInfoTree(shared_ptr<TreeItem>& root, QObject* parent = nullptr)
        : QAbstractItemModel(parent), m_frame_info(root) {
    }

    // 行数
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        TreeItem* parentItem;
        if (parent.isValid()) {
            parentItem = static_cast<TreeItem*>(parent.internalPointer());
        } else {
            // 如果是根节点，使用 m_frame_info
            parentItem = m_frame_info.get();
        }

        return parentItem ? parentItem->childCount() : 0;
    }

    // 列数
    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        return 2; // "Name" 和 "Description"
    }

    // 获取头部数据
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            if (section == 0)
                return "字段";
            if (section == 1)
                return "值";
        }
        return {};
    }

    // 获取数据
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // 获取父节点 (树形结构一般是无父节点的情况下)
    QModelIndex parent(const QModelIndex& index) const override;

    // 获取子节点
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

  private:
    shared_ptr<TreeItem> m_frame_info;
};

/**
 * @class ModelFrameBinary
 * @brief 继承QAbstractTableModel，和tableView绑定
 */
class ModelFrameBinary : public QAbstractTableModel {
    Q_OBJECT
  public:
    explicit ModelFrameBinary(BinaryData& data, QObject* parent = nullptr) : QAbstractTableModel(parent), m_data(data) {
    }

    // read
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    static const int column_size = 5;

    // 获取数据
    BinaryData& getData() {
        return m_data;
    }

  private:
    BinaryData m_data;
};

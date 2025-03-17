#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include <QFile>
#include <QAbstractItemModel>
#include "frameinfo.h"
using namespace std;

/**
 * @class ModelFrameList
 * @brief 继承QAbstractTableModel，和tableView绑定
 */
class ModelFrameList : public QAbstractTableModel
{
    Q_OBJECT
public:
    // view相关
    explicit ModelFrameList(
        const vector<shared_ptr<ModelFLVFrame>> &list, QObject *parent = nullptr)
        : QAbstractTableModel(parent), m_frameList(list)
    {
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // 数据相关
    vector<shared_ptr<ModelFLVFrame>> getFrameList() { return m_frameList; }
    int readFromFile(QFile &path);
    static const int column_size = 5;

private:
    vector<shared_ptr<ModelFLVFrame>> m_frameList;
};

/**
 * @class ModelFrameInfoTree
 * @brief 继承QAbstractItemModel，和treeView绑定
 */
class ModelFrameInfoTree : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ModelFrameInfoTree(
        TreeItem *frame, QObject *parent = nullptr)
        : QAbstractItemModel(parent), rootItem(frame)
    {
    }

    // 行数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        TreeItem *parentItem = parent.isValid() ? static_cast<TreeItem *>(parent.internalPointer())
                                                : rootItem.get();
        if (parentItem)
            return parentItem->childCount();
        return 0;
    }

    // 列数
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return 2; // "Name" 和 "Description"
    }

    // 获取头部数据
    QVariant headerData(
        int section, Qt::Orientation orientation, int role) const override
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            if (section == 0)
                return "字段";
            if (section == 1)
                return "值";
        }
        return QVariant();
    }

    // 获取数据
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // 获取父节点 (树形结构一般是无父节点的情况下)
    QModelIndex parent(const QModelIndex &index) const override;

    // 获取子节点
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

private:
    unique_ptr<TreeItem> rootItem;
};

/**
 * @class ModelFrameBinary
 * @brief 继承QAbstractTableModel，和tableView绑定
 */
class ModelFrameBinary : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ModelFrameBinary(
        BinaryData &data, QObject *parent = nullptr)
        : QAbstractTableModel(parent), m_data(data)
    {
    }

    // read
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    static const int column_size = 5;

private:
    BinaryData m_data;
};

#endif // MODELFRAMEINFO_H
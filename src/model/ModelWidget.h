// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include "taginfo.h"
#include <QAbstractItemModel>
#include <QFile>
using namespace std;

/**
 * @class ModelTagList
 * @brief 继承QAbstractTableModel，和tableView绑定
 */
class ModelTagList : public QAbstractTableModel {
    Q_OBJECT
  public:
    // view相关
    explicit ModelTagList(QObject* parent = nullptr) : QAbstractTableModel(parent) {
    }
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 数据相关
    FLVHeader* getFlvHeader() {
        return m_flv_header.get();
    }
    vector<unique_ptr<FLVTag>>& getTagList() {
        return m_tagList;
    }

    int readFromFile(QFile& path);
    static const int column_size = 5;

    // 添加删除方法
    bool removeRow(int row, const QModelIndex& parent = QModelIndex()) {
        beginRemoveRows(parent, row, row);
        m_tagList.erase(m_tagList.begin() + row);
        endRemoveRows();
        return true;
    }

  private:
    unique_ptr<FLVHeader> m_flv_header;
    vector<unique_ptr<FLVTag>> m_tagList;
};

/**
 * @class ModelTagInfoTree
 * @brief 继承QAbstractItemModel，和treeView绑定
 */
class ModelTagInfoTree : public QAbstractItemModel {
    Q_OBJECT
  public:
    explicit ModelTagInfoTree(shared_ptr<TreeItem>& root, QObject* parent = nullptr)
        : QAbstractItemModel(parent), m_tag_info(root) {
    }

    // 行数
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        TreeItem* parentItem;
        if (parent.isValid()) {
            parentItem = static_cast<TreeItem*>(parent.internalPointer());
        } else {
            // 如果是根节点，使用 m_tag_info
            parentItem = m_tag_info.get();
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
    shared_ptr<TreeItem> m_tag_info;
};

/**
 * @class ModelTagBinary
 * @brief 继承QAbstractTableModel，和tableView绑定
 */
class ModelTagBinary : public QAbstractTableModel {
    Q_OBJECT
  public:
    explicit ModelTagBinary(BinaryData& data, QObject* parent = nullptr) : QAbstractTableModel(parent), m_data(data) {
    }

    // 设置文件路径（用于编辑后保存）
    void setFilePath(const QString& filePath) {
        m_filePath = filePath;
    }

    // read
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 设置数据（编辑功能）
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    // 返回项的标志（设置可编辑）
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    static const int column_size = 5;

    // 获取数据
    BinaryData& getData() {
        return m_data;
    }

  signals:
    void dataModified();

  private:
    BinaryData m_data;
    QString m_filePath;
};

// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "tagview.h"
#include "BinaryEditDelegate.h"
#include "ui_tagview.h"
#include <QAction>
#include <QMenu>
#include <QMessageBox>

TagView::TagView(QWidget* parent) : QWidget(parent), ui(new Ui::TagView) {
    ui->setupUi(this);
    setupConnections();
}

TagView::~TagView() {
    delete ui;
}

void TagView::setupConnections() {
    // 创建右键菜单
    m_contextMenu = new QMenu(this);
    m_deleteAction = new QAction("删除帧", this);
    m_contextMenu->addAction(m_deleteAction);

    // 右键菜单
    connect(ui->tagTableView, &QTableView::customContextMenuRequested, this, &TagView::showContextMenu);
    connect(m_deleteAction, &QAction::triggered, this, &TagView::handleDeleteTag);
}

void TagView::setTagList(unique_ptr<ModelTagList> model) {
    m_tag_table_model = std::move(model);
    ui->tagTableView->setModel(m_tag_table_model.get());
    m_tag_info_tree.reset();
    m_tag_data.reset();

    // 获取选中模型，并连接选中变化信号
    QItemSelectionModel* selectionModel = ui->tagTableView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &TagView::onTagSelectionChanged);
}

void TagView::clearTagList() {
    m_tag_table_model.reset();
    ui->tagTableView->setModel(nullptr);
    m_tag_info_tree.reset();
    m_tag_data.reset();
}

void TagView::showContextMenu(const QPoint& pos) {
    QModelIndex index = ui->tagTableView->indexAt(pos);
    if (index.isValid()) {
        m_contextMenu->exec(ui->tagTableView->viewport()->mapToGlobal(pos));
    }
}

void TagView::handleDeleteTag() {
    QModelIndex currentIndex = ui->tagTableView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    // 弹出确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "确认删除",
                                                              "确定要删除选中的帧吗？\n\n此操作不可恢复！",
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        emit tagDeleteRequested(currentIndex.row());
    }
}

void TagView::onTagSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    QModelIndexList selectedIndexes = selected.indexes();
    if (selectedIndexes.isEmpty()) {
        return;
    }

    QModelIndex index = selectedIndexes.first();
    int row = index.row();
    if (row < 0) {
        return;
    }

    if (m_tag_table_model == nullptr || row > m_tag_table_model->getTagList().size()) {
        return;
    }

    BinaryData* data_ptr = nullptr;
    if (row == 0) {
        auto header = m_tag_table_model->getFlvHeader();
        m_tag_info_tree.reset(new ModelTagInfoTree(header->getTreeInfo()));
        data_ptr = static_cast<BinaryData*>(header);
    } else {
        auto tag = m_tag_table_model->getTagList()[row - 1].get();
        m_tag_info_tree.reset(new ModelTagInfoTree(tag->getTreeInfo()));
        data_ptr = static_cast<BinaryData*>(tag);
    }

    // 帧详细信息视图
    ui->tagInfoTree->setModel(m_tag_info_tree.get());

    // 展开根下的所有第一层索引（显示第二层）
    for (int row = 0; row < m_tag_info_tree->rowCount(QModelIndex()); ++row) {
        QModelIndex idx = m_tag_info_tree->index(row, 0, QModelIndex());
        ui->tagInfoTree->expand(idx);
    }

    // 连接字段选择变化
    QItemSelectionModel* fieldSel = ui->tagInfoTree->selectionModel();
    if (fieldSel) {
        connect(fieldSel, &QItemSelectionModel::selectionChanged, this, &TagView::onFieldSelectionChanged);
    }

    // 帧二进制数据视图
    m_tag_data.reset(new ModelTagBinary(*data_ptr));
    m_tag_data->setFilePath(m_filePath);
    ui->tagRawContent->setModel(m_tag_data.get());

    // 连接数据修改信号
    connect(m_tag_data.get(), &ModelTagBinary::dataModified, this, &TagView::onBinaryDataModified);

    // 设置编辑委托（用于确认框）
    ui->tagRawContent->setItemDelegate(new BinaryEditDelegate(ui->tagRawContent));

    // 启用双击编辑
    ui->tagRawContent->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
}

void TagView::onBinaryDataModified() {
    // 发出文件修改信号，由 MainWindow 处理重新加载
    emit fileModified();
}

void TagView::onFieldSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    QModelIndexList selectedIndexes = selected.indexes();
    if (selectedIndexes.isEmpty()) {
        return;
    }

    QModelIndex index = selectedIndexes.first();
    auto item = static_cast<TreeItem*>(index.internalPointer());

    int64_t offset = item->data->offset;
    uint32_t size = item->data->size;

    if (m_tag_data == nullptr) {
        return;
    }

    if (offset <= 0)
        offset = -offset;
    else
        offset -= m_tag_data->getData().m_offset;

    if (offset < 0 || static_cast<uint64_t>(offset) >= m_tag_data->getData().m_size) {
        return;
    }

    // 确保表格使用按单元格选择模式
    ui->tagRawContent->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tagRawContent->setSelectionBehavior(QAbstractItemView::SelectItems);

    QItemSelectionModel* sel = ui->tagRawContent->selectionModel();
    if (!sel) {
        return;
    }

    sel->clearSelection();

    if (size == 0) {
        return;
    }

    int64_t endRel = offset + size;
    if (endRel > m_tag_data->getData().m_size)
        endRel = m_tag_data->getData().m_size;

    // 直接计算左上 (startRow,startCol) 与右下 (endRow,endCol) 索引并用单个矩形范围选中
    int startRow = static_cast<int>(offset / 16);
    int startCol = static_cast<int>(offset % 16);
    int endRow = static_cast<int>((endRel - 1) / 16);
    int endCol = static_cast<int>((endRel - 1) % 16);

    QModelIndex topLeft;
    QModelIndex bottomRight;

    if (startRow + 1 < endRow) {
        topLeft = ui->tagRawContent->model()->index(startRow + 1, 0);
        bottomRight = ui->tagRawContent->model()->index(endRow - 1, 15);
        if (topLeft.isValid() && bottomRight.isValid()) {
            sel->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
        }
    }

    if (startRow < endRow) {
        topLeft = ui->tagRawContent->model()->index(endRow, 0);
        bottomRight = ui->tagRawContent->model()->index(endRow, endCol);
        if (topLeft.isValid() && bottomRight.isValid()) {
            sel->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
        }

        topLeft = ui->tagRawContent->model()->index(startRow, startCol);
        bottomRight = ui->tagRawContent->model()->index(startRow, 15);
        if (topLeft.isValid() && bottomRight.isValid()) {
            sel->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
        }
    } else if (startRow == endRow) {
        topLeft = ui->tagRawContent->model()->index(startRow, startCol);
        bottomRight = ui->tagRawContent->model()->index(endRow, endCol);
        if (topLeft.isValid() && bottomRight.isValid()) {
            sel->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
        }
    }

    if (topLeft.isValid()) {
        ui->tagRawContent->scrollTo(topLeft);
    }
}

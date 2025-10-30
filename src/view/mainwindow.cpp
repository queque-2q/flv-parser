#include "mainwindow.h"
#include "DeleteStrategy.h"
#include "Log.h"
#include "Utils.h"
#include "frameinfo.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <cstring>
#include <memory>

using namespace std;

/**
 @class MainWindow
*/

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setupContextMenu();

    // 全局统一选中颜色：设置应用级调色板和样式，使所有视图元素在选中时使用相同的强蓝色并显示白色文字
    // 颜色调暗一点，避免过于刺眼
    QColor strongBlue(20, 100, 200); // 较暗的蓝色
    QPalette appPal = qApp->palette();
    appPal.setColor(QPalette::Highlight, strongBlue);
    appPal.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(appPal);

    // 作为补充，设置全局 stylesheet，确保 item:selected 在所有视图（Table/Tree/List）中都使用同样样式
    QString selStyle = QString("QTableView::item:selected, QTreeView::item:selected, QListView::item:selected, "
                               "QHeaderView::section:selected { background-color: rgb(%1,%2,%3); color: white; }")
                           .arg(strongBlue.red())
                           .arg(strongBlue.green())
                           .arg(strongBlue.blue());
    qApp->setStyleSheet(qApp->styleSheet() + selStyle);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupContextMenu() {
    // 创建右键菜单
    contextMenu = new QMenu(this);
    deleteAction = new QAction("删除帧", this);
    contextMenu->addAction(deleteAction);

    // 连接删除动作的信号槽
    connect(deleteAction, &QAction::triggered, this, &MainWindow::handleDeleteFrame);

    // 为tableView添加右键菜单
    ui->frameTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->frameTableView, &QTableView::customContextMenuRequested, this, [this](const QPoint& pos) {
        QModelIndex index = ui->frameTableView->indexAt(pos);
        if (index.isValid()) {
            contextMenu->exec(ui->frameTableView->viewport()->mapToGlobal(pos));
        }
    });
}

void MainWindow::handleDeleteFrame() {
    // 获取当前选中的行
    QModelIndex currentIndex = ui->frameTableView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    // 弹出确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "确认删除",
                                                              "确定要删除选中的帧吗？此操作不可恢复！",
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::No // 默认选择"否"
    );

    if (reply == QMessageBox::Yes) {
        // 删除选中的行
        auto strategy = FrameDeleteStrategyFactory::createStrategy(m_currentFile);

        // 执行删除操作
        if (strategy->deleteFrame(m_currentFile, currentIndex.row(), m_frame_table_model->getFrameList())) {
            // 删除成功后重新加载文件
            loadFile();
        }
    }
}

void MainWindow::on_actionopen_triggered() {
    // 读文件
    QString fileName = QFileDialog::getOpenFileName(this, "Open the file");
    if (fileName.isEmpty())
        return;
    m_currentFile = fileName;
    loadFile();
}

void MainWindow::loadFile() {
    QFile file(m_currentFile);

    if (!file.open(QFile::ReadWrite)) {
        QMessageBox::warning(this, "Warning", "Cannot open file: " + file.errorString());
        return;
    }
    setWindowTitle(m_currentFile);

    // 解析文件
    m_frame_table_model.reset(new ModelFrameList({}));
    try {
        m_frame_table_model->readFromFile(file);
    } catch (const QString& error) {
        if (m_frame_table_model->getFrameList().empty()) {
            QMessageBox::warning(this, "Warning", "解析失败：" + file.errorString());
            return;
        }
    }

    file.close();

    // frameTableView展示帧信息
    auto tableView = qobject_cast<QTableView*>(findChild<QObject*>("frameTableView"));
    if (tableView != nullptr) {
        tableView->setModel(m_frame_table_model.get());

        // 获取选中模型，并连接选中变化信号
        QItemSelectionModel* selectionModel = tableView->selectionModel();
        connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &MainWindow::onFrameSelectionChanged);
        tableView->show();
    }
}

void MainWindow::on_actionsave_triggered() {
}

void MainWindow::on_actionsave_as_triggered() {
}

void MainWindow::on_actionhelp_triggered() {
    QString helpText = "支持以下功能：\n"
                       "1. 解析flv文件\n"
                       "2. 删除flv帧";
    QMessageBox::information(this, "帮助", helpText);
}

void MainWindow::on_actionabout_triggered() {
    QString aboutText = "作者：2Q\n"
                        "版本：1.0\n"
                        "说明：flv parser";
    QMessageBox::information(this, "关于", aboutText);
}

void MainWindow::onFrameSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    // 获取选中的行
    QModelIndexList selectedIndexes = selected.indexes();
    if (selectedIndexes.isEmpty()) {
        return;
    }

    QModelIndex index = selectedIndexes.first();
    int row = index.row();
    qCDebug(runLog) << "Selected Row:" << row;
    if (row < 0) {
        return;
    }

    // 帧列表
    if (m_frame_table_model == nullptr || row >= m_frame_table_model->getFrameList().size()) {
        return;
    }
    auto frame = m_frame_table_model->getFrameList()[row];

    // 帧详细信息视图
    m_frame_info_tree.reset(new ModelFrameInfoTree(frame->getTreeInfo())); // 必须重新new，否则model绑定的数据不会更新

    auto treeView = qobject_cast<QTreeView*>(findChild<QObject*>("frameInfoTree"));
    if (treeView != nullptr) {
        treeView->setModel(m_frame_info_tree.get());

        // 展开根下的所有第一层索引（显示第二层）
        for (int row = 0; row < m_frame_info_tree->rowCount(QModelIndex()); ++row) {
            QModelIndex idx = m_frame_info_tree->index(row, 0, QModelIndex());
            treeView->expand(idx);
        }
        // 连接 field selection 的变化，只在 model 设置后连接（selectionModel 会在 setModel 时创建）
        QItemSelectionModel* fieldSel = treeView->selectionModel();
        if (fieldSel) {
            connect(fieldSel, &QItemSelectionModel::selectionChanged, this, &MainWindow::onFieldSelectionChanged);
        }

        treeView->show();
    }

    // 帧二进制数据视图
    m_frame_data.reset(new ModelFrameBinary(*(static_cast<BinaryData*>(frame.get()))));

    auto tableView = qobject_cast<QTableView*>(findChild<QObject*>("frameRawContent"));
    if (tableView != nullptr) {
        tableView->setModel(m_frame_data.get());
        tableView->show();
    }
}

void MainWindow::onFieldSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    // 获取选中的行
    QModelIndexList selectedIndexes = selected.indexes();
    if (selectedIndexes.isEmpty()) {
        return;
    }

    QModelIndex index = selectedIndexes.first();
    auto item = static_cast<TreeItem*>(index.internalPointer());
    qCDebug(runLog) << "Selected Row:" << item->data->name;

    uint64_t offset = item->data->offset;
    uint32_t size = item->data->size;
    if (size == 0) {
        return;
    }

    // 帧信息
    if (m_frame_data == nullptr || offset >= m_frame_data->getData().m_size) {
        return;
    }

    // 帧二进制数据视图frameRawContent 设置对应数据高亮
    auto tableView = qobject_cast<QTableView*>(findChild<QObject*>("frameRawContent"));
    if (tableView == nullptr) {
        return;
    }

    // 确保表格使用按单元格选择模式
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectItems);

    QItemSelectionModel* sel = tableView->selectionModel();
    if (!sel) {
        // 正常情况下在 setModel 之后会有 selectionModel，
        // 这里做保护以防万一（例如 model 未设置或被清除），直接返回即可。
        return;
    }

    // sel 已存在，继续后续处理
    sel->clearSelection();

    uint64_t endRel = offset + size;
    if (endRel > m_frame_data->getData().m_size)
        endRel = m_frame_data->getData().m_size;

    // 直接计算左上 (startRow,startCol) 与右下 (endRow,endCol) 索引并用单个矩形范围选中
    int startRow = static_cast<int>(offset / 16);
    int startCol = static_cast<int>(offset % 16);
    int endRow = static_cast<int>((endRel - 1) / 16);
    int endCol = static_cast<int>((endRel - 1) % 16);

    QModelIndex topLeft;
    QModelIndex bottomRight;

    if (startRow + 1 < endRow) {
        topLeft = tableView->model()->index(startRow + 1, 0);
        bottomRight = tableView->model()->index(endRow - 1, 15);
        if (topLeft.isValid() && bottomRight.isValid()) {
            sel->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
        }
    }

    if (startRow < endRow) {
        // 处理相邻或同一行的情况，按实际的列范围选择
        topLeft = tableView->model()->index(endRow, 0);
        bottomRight = tableView->model()->index(endRow, endCol);
        if (topLeft.isValid() && bottomRight.isValid()) {
            sel->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
        }

        topLeft = tableView->model()->index(startRow, startCol);
        bottomRight = tableView->model()->index(startRow, 15);
        if (topLeft.isValid() && bottomRight.isValid()) {
            sel->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
        }
    } else if (startRow == endRow) {
        topLeft = tableView->model()->index(startRow, startCol);
        bottomRight = tableView->model()->index(endRow, endCol);
        if (topLeft.isValid() && bottomRight.isValid()) {
            sel->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
        }
    }

    if (topLeft.isValid()) {
        tableView->scrollTo(topLeft, QAbstractItemView::PositionAtCenter);
    }
}

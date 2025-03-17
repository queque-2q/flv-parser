/**
 * @file mainwindow.cpp
 * @brief 主窗口类的实现文件
 * @details 实现了应用程序的主窗口界面，包括：
 *          - FLV文件的打开和保存
 *          - 帧列表的展示
 *          - 帧详细信息的树形展示
 *          - 帧二进制数据的展示
 * @author Your Name
 * @date 2024-03-11
 */

#pragma execution_character_set("utf-8")

#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include "frameinfo.h"
#include "ui_mainwindow.h"
#include <cstring>
#include <memory>

using namespace std;

/**
 @class MainWindow
*/

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionopen_triggered()
{
    //读文件
    QString fileName = QFileDialog::getOpenFileName(this, "Open the file");
    if (fileName.isEmpty())
        return;
    QFile file(fileName);
    currentFile = fileName;
    if (!file.open(QFile::ReadWrite)) {
        QMessageBox::warning(this, "Warning", "Cannot open file: " + file.errorString());
        return;
    }
    setWindowTitle(fileName);

    //解析文件
    m_frame_table_model.reset(new ModelFrameList({}));
    try {
        m_frame_table_model->readFromFile(file);
    } catch (const QString &error) {
        if (m_frame_table_model->getFrameList().empty()) {
            QMessageBox::warning(this, "Warning", "解析失败：" + file.errorString());
            return;
        }
    }

    file.close();

    //frameTableView展示帧信息
    QTableView *tableView = qobject_cast<QTableView *>(findChild<QObject *>("frameTableView"));
    if (tableView) {
        tableView->setModel(m_frame_table_model.get());

        //获取选中模型，并连接选中变化信号
        QItemSelectionModel *selectionModel = tableView->selectionModel();
        connect(selectionModel,
                &QItemSelectionModel::selectionChanged,
                this,
                &MainWindow::onFrameSelectionChanged);
        tableView->show();
    }
}

void MainWindow::on_actionsave_triggered() {}

void MainWindow::on_actionsave_as_triggered() {}

void MainWindow::on_actionhelp_triggered() {}

void MainWindow::on_actionabout_triggered() {}

void MainWindow::onFrameSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    // 获取选中的行
    QModelIndexList selectedIndexes = selected.indexes();
    if (!selectedIndexes.isEmpty()) {
        QModelIndex index = selectedIndexes.first();
        int row = index.row();

        // 获取选中行的相关信息
        qDebug() << "Selected Row:" << row;

        //帧详细信息视图
        auto frame = m_frame_table_model->getFrameList()[row];
        if (frame->a_info != nullptr) {
            m_frame_info_tree.reset(new ModelFrameInfoTree(frame->a_info->toTreeObj()));
        } else if (frame->v_info != nullptr) {
            m_frame_info_tree.reset(new ModelFrameInfoTree(frame->v_info->toTreeObj()));
        } else if(frame->metadata_info != nullptr){
            m_frame_info_tree.reset(new ModelFrameInfoTree(frame->metadata_info->toTreeObj()));
        }
        else{
            m_frame_info_tree.reset(new ModelFrameInfoTree(nullptr));
        }

        QTreeView *treeView = qobject_cast<QTreeView *>(findChild<QObject *>("frameInfoTree"));
        if (treeView) {
            treeView->setModel(m_frame_info_tree.get());
            treeView->show();
        }

        //帧二进制数据视图
        m_frame_data.reset(new ModelFrameBinary(*(static_cast<BinaryData *>(frame.get()))));

        // 展示到二进制框中
        QTableView *tableView = qobject_cast<QTableView *>(findChild<QObject *>("frameRawContent"));
        if (tableView) {
            tableView->setModel(m_frame_data.get());
            tableView->show();
        }
    }
}

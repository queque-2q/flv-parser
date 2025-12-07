// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "mainwindow.h"
#include "DeleteStrategy.h"
#include "Log.h"
#include "docview.h"
#include "logview.h"
#include "tagview.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <memory>

using namespace std;

/**
 @class MainWindow
*/

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setupViews();

    // 全局统一选中颜色：设置应用级调色板和样式，使所有视图元素在选中时使用相同的强蓝色并显示白色文字
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

void MainWindow::handleTagDelete(int row) {
    auto strategy = TagDeleteStrategyFactory::createStrategy(m_currentFile);

    // 执行删除操作
    if (m_tagView && m_tagView->getTagModel() &&
        strategy->deleteTag(m_currentFile, row, m_tagView->getTagModel()->getTagList())) {
        // 删除成功后重新加载文件
        qCInfo(runLog) << "[flv-parsing] event[file start reloading]";
        loadFile();
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
    m_stackedWidget->setCurrentWidget(m_tagView);

    QFile file(m_currentFile);

    if (!file.open(QFile::ReadWrite)) {
        QMessageBox::warning(this, "Warning", "Cannot open file: " + file.errorString());
        return;
    }
    setWindowTitle(m_currentFile);

    // 在状态栏提示正在处理
    m_tagView->clearTagList(); // 清除旧数据

    QLabel* statusLabel = new QLabel("⏳ Processing...");
    statusLabel->setStyleSheet("color: #0066cc; font-weight: bold;");
    statusBar()->addWidget(statusLabel);
    QApplication::processEvents(); // 强制刷新UI，让提示立即显示

    // 解析文件
    auto tag_table_model = make_unique<ModelTagList>();
    tag_table_model->readFromFile(file);
    file.close();

    // 设置帧列表到视图
    if (m_tagView) {
        m_tagView->setFilePath(m_currentFile);
        m_tagView->setTagList(std::move(tag_table_model));
    }

    // 处理完成后清除提示
    statusBar()->clearMessage();
    statusBar()->removeWidget(statusLabel);
}

void MainWindow::on_actionabout_triggered() {
    QString aboutText = "作者：2Q\n"
                        "版本：" VERSION_STRING "\n"
                        "说明：flv parser";
    QMessageBox::information(this, "关于", aboutText);
}

void MainWindow::on_actionViewLog_triggered() {
    if (m_stackedWidget && m_logView) {
        m_stackedWidget->setCurrentWidget(m_logView);
        m_logView->loadLogFile();
    }
}

void MainWindow::on_actionViewMain_triggered() {
    if (m_stackedWidget && m_tagView) {
        m_stackedWidget->setCurrentWidget(m_tagView);
    }
}

void MainWindow::on_actionViewDoc_triggered() {
    if (m_stackedWidget && m_docView) {
        m_stackedWidget->setCurrentWidget(m_docView);
        m_docView->loadDocument();
    }
}

void MainWindow::setupViews() {
    // 创建StackedWidget作为中央widget
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // 创建帧视图
    m_tagView = new TagView(this);
    m_stackedWidget->addWidget(m_tagView);

    // 连接帧删除信号
    connect(m_tagView, &TagView::tagDeleteRequested, this, &MainWindow::handleTagDelete);

    // 连接文件修改信号
    connect(m_tagView, &TagView::fileModified, this, &MainWindow::loadFile);

    // 创建日志查看界面
    m_logView = new LogView(this);
    m_stackedWidget->addWidget(m_logView);

    // 创建文档查看界面
    m_docView = new DocView(this);
    m_stackedWidget->addWidget(m_docView);

    // 默认显示帧视图
    m_stackedWidget->setCurrentIndex(0);
}

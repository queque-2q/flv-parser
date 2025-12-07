// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include "modelwidget.h"
#include <QAction>
#include <QItemSelection>
#include <QMainWindow>
#include <QMenu>

using namespace std;

#define VERSION_STRING "1.0.3"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QStackedWidget;
class LogView;
class TagView;
class DocView;

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

  private slots:
    void on_actionopen_triggered();

    void on_actionabout_triggered();

    void on_actionViewLog_triggered();

    void on_actionViewMain_triggered();

    void on_actionViewDoc_triggered();

    void handleTagDelete(int row);

  private:
    void loadFile();
    void setupViews();

  private:
    Ui::MainWindow* ui;
    QString m_currentFile;

    // 视图管理
    QStackedWidget* m_stackedWidget;
    TagView* m_tagView;
    LogView* m_logView;
    DocView* m_docView;
};

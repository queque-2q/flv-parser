#pragma once

#include "modelwidget.h"
#include <QAction>
#include <QItemSelection>
#include <QMainWindow>
#include <QMenu>

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

  private slots:
    void on_actionopen_triggered();

    void on_actionsave_triggered();

    void on_actionsave_as_triggered();

    void on_actionhelp_triggered();

    void on_actionabout_triggered();

    void onFrameSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onFieldSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  private:
    void setupContextMenu();
    void handleDeleteFrame();
    void loadFile();

  private:
    Ui::MainWindow* ui;
    QString m_currentFile;
    unique_ptr<ModelFrameList> m_frame_table_model;
    unique_ptr<ModelFrameInfoTree> m_frame_info_tree;
    unique_ptr<ModelFrameBinary> m_frame_data;
    QMenu* contextMenu;
    QAction* deleteAction;
};

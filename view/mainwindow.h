#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QItemSelection>
#include <QMainWindow>
#include "modelwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionopen_triggered();

    void on_actionsave_triggered();

    void on_actionsave_as_triggered();

    void on_actionhelp_triggered();

    void on_actionabout_triggered();

    void onFrameSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    Ui::MainWindow *ui;
    QString currentFile;
    std::unique_ptr<ModelFrameList> m_frame_table_model;
    std::unique_ptr<ModelFrameInfoTree> m_frame_info_tree;
    std::unique_ptr<ModelFrameBinary> m_frame_data;
};
#endif // MAINWINDOW_H

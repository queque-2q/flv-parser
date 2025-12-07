// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include "modelwidget.h"
#include <QItemSelection>
#include <QWidget>
#include <memory>

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui {
class TagView;
}
QT_END_NAMESPACE

class TagView : public QWidget {
    Q_OBJECT

  public:
    explicit TagView(QWidget* parent = nullptr);
    ~TagView();

    void clearTagList();
    void setTagList(unique_ptr<ModelTagList> model);
    ModelTagList* getTagModel() const {
        return m_tag_table_model.get();
    }

    void setFilePath(const QString& filePath) {
        m_filePath = filePath;
    }

  signals:
    void tagDeleteRequested(int row);
    void fileModified();

  private slots:
    void onTagSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onFieldSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void showContextMenu(const QPoint& pos);
    void handleDeleteTag();
    void onBinaryDataModified();

  private:
    void setupConnections();

  private:
    Ui::TagView* ui;

    unique_ptr<ModelTagList> m_tag_table_model;
    unique_ptr<ModelTagInfoTree> m_tag_info_tree;
    unique_ptr<ModelTagBinary> m_tag_data;

    QMenu* m_contextMenu;
    QAction* m_deleteAction;

    QString m_filePath;
};

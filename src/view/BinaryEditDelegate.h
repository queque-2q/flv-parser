// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QStyledItemDelegate>

class BinaryEditDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    explicit BinaryEditDelegate(QObject* parent = nullptr);

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
};

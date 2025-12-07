// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

namespace Ui {
class DocView;
}

class DocView : public QWidget {
    Q_OBJECT

  public:
    explicit DocView(QWidget* parent = nullptr);
    ~DocView();

    void loadDocument();

  private:
    Ui::DocView* ui;
};

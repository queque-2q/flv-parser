// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class LogView;
}
QT_END_NAMESPACE

class LogView : public QWidget {
    Q_OBJECT

  public:
    explicit LogView(QWidget* parent = nullptr);
    ~LogView();

    void loadLogFile();

  private slots:
    void filterLogByLevel(const QString& level);

  private:
    Ui::LogView* ui;
    QString m_fullLogContent;
};

// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "logview.h"
#include "ui_logview.h"
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

LogView::LogView(QWidget* parent) : QWidget(parent), ui(new Ui::LogView) {
    ui->setupUi(this);

    // 连接下拉框的信号
    connect(ui->logLevelCombo, &QComboBox::currentTextChanged, this, &LogView::filterLogByLevel);
}

LogView::~LogView() {
    delete ui;
}

void LogView::loadLogFile() {
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/flv_parser.log";
    QFile logFile(logPath);

    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->logTextEdit->setPlainText("无法打开日志文件: " + logPath);
        return;
    }

    QTextStream in(&logFile);
    m_fullLogContent = in.readAll();
    logFile.close();

    // 应用当前选择的过滤器
    filterLogByLevel(ui->logLevelCombo->currentText());
}

void LogView::filterLogByLevel(const QString& level) {
    if (m_fullLogContent.isEmpty()) {
        return;
    }

    if (level == "全部") {
        ui->logTextEdit->setPlainText(m_fullLogContent);
        return;
    }

    // 根据日志等级过滤
    QStringList lines = m_fullLogContent.split('\n');
    QStringList filteredLines;

    // Qt日志格式为: [debug], [warning], [info]
    if (level == "Warning") {
        for (const QString& line : lines) {
            if (line.contains("[warning]", Qt::CaseInsensitive)) {
                filteredLines.append(line);
            }
        }
    } else if (level == "Info") {
        for (const QString& line : lines) {
            if (line.contains("[info]", Qt::CaseInsensitive) || line.contains("[warning]", Qt::CaseInsensitive)) {
                filteredLines.append(line);
            }
        }
    } else if (level == "Debug") {
        for (const QString& line : lines) {
            filteredLines.append(line);
        }
    }

    ui->logTextEdit->setPlainText(filteredLines.join('\n'));
}

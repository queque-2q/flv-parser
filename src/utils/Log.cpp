// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "Log.h"
#include <QFile>
#include <QMutex>
#include <QStandardPaths>
#include <QTextStream>

Q_LOGGING_CATEGORY(runLog, "runLog")

// 定义全局文件对象和锁（确保线程安全）
static QFile g_logFile;
static QMutex g_logMutex;

void initLog() {
    QMutexLocker locker(&g_logMutex);

    // 设置日志路径（Windows 建议写入用户目录）
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/flv_parser.log";
    g_logFile.setFileName(logPath);
    if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << "无法打开日志文件!";
    }
}

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QMutexLocker locker(&g_logMutex);

    // 打开文件（如果未打开）
    if (!g_logFile.isOpen()) {
        return;
    }

    // 格式化日志内容
    QString logMsg = qFormatLogMessage(type, context, msg);

    QTextStream stream(&g_logFile);
    stream << logMsg << Qt::endl;
}

void printLogWithPos(QtMsgType type, const QDataStream& stream, const QString& msg) {
    QString logMsg = QString("[flv-parsing] file-pos[0x%1] %2").arg(stream.device()->pos(), 0, 16).arg(msg);
    qt_message_output(type, QMessageLogContext(), logMsg);
}
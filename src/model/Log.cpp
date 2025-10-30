#include "Log.h"
#include <QFile>
#include <QMutex>
#include <QStandardPaths>
#include <QTextStream>

Q_LOGGING_CATEGORY(runLog, "runLog")

// 定义全局文件对象和锁（确保线程安全）
static QFile g_logFile;
static QMutex g_logMutex;

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QMutexLocker locker(&g_logMutex);

    // 打开文件（如果未打开）
    if (!g_logFile.isOpen()) {
        // 设置日志路径（Windows 建议写入用户目录）
        QString logPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/flv_parser.log";
        g_logFile.setFileName(logPath);
        if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            qDebug() << "无法打开日志文件!";
            return;
        }
    }

    // 格式化日志内容
    QString logMsg = qFormatLogMessage(type, context, msg);

    QTextStream stream(&g_logFile);
    stream << logMsg << Qt::endl;
}

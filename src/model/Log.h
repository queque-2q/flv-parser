#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(runLog)

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
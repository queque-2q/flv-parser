// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(runLog)

void initLog();
void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
void printLogWithPos(QtMsgType type, const QDataStream& stream, const QString& msg);
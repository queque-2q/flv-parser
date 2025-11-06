// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "Log.h"
#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/app.ico"));
    qInstallMessageHandler(customMessageHandler);
    MainWindow w;
    w.show();
    return a.exec();
}

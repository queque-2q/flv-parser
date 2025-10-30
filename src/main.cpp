#include "Log.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    qInstallMessageHandler(customMessageHandler);
    MainWindow w;
    w.show();
    return a.exec();
}

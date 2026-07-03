#include "mainwindow.h"
#include "eventlogger.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    EventLogger::instance(&a);
    MainWindow w;
    w.showMaximized();
    return a.exec();
}

#include "mainwindow.h"
#include "eventlogger.h"
#include "appsettings.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Q_INIT_RESOURCE(breeze);
    a.setWindowIcon(QIcon(":/resources/appIcon.png"));
    QImage image(":/resources/appIcon.png");

    qDebug() << "hasAlphaChannel:" << image.hasAlphaChannel();

    const AppSettings::Settings settings = AppSettings::load();
    AppSettings::applyTheme(a, settings.theme);

    EventLogger::instance(&a);
    MainWindow w;
    w.showMaximized();
    return a.exec();
}

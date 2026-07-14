#include "mainwindow.h"
#include "eventlogger.h"
#include "appsettings.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Q_INIT_RESOURCE(breeze);
    a.setWindowIcon(QIcon(":/resources/appIcon.png"));

    const AppSettings::Settings settings = AppSettings::load();
    AppSettings::applyTheme(a, settings.theme);

    auto *translator = new QTranslator(&a);
    translator->setObjectName(QStringLiteral("applicationTranslator"));
    AppSettings::applyLanguage(a, settings.language, *translator);

    EventLogger::instance(&a);
    MainWindow w;
    w.showMaximized();
    return a.exec();
}

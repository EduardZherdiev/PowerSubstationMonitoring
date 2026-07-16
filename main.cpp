#include "appsettings.h"
#include "eventlogger.h"
#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    Q_INIT_RESOURCE(breeze);
    Q_INIT_RESOURCE(resources);
    a.setWindowIcon(QIcon(":/resources/appIcon.png"));

    const AppSettings::Settings settings = AppSettings::load();
    AppSettings::applyTheme(a, settings.theme);

    auto* translator = new QTranslator(&a);
    translator->setObjectName(QStringLiteral("applicationTranslator"));
    AppSettings::applyLanguage(a, settings.language, *translator);

    EventLogger::instance(&a);
    MainWindow w;
    w.showMaximized();
    return a.exec();
}

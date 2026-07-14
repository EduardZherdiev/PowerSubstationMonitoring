#include "appsettings.h"
#include "diagramtheme.h"
#include "substationdiagramview.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QWidget>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QTranslator>

namespace AppSettings {

QString settingsFilePath()
{
    const QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(basePath);
    return basePath + QStringLiteral("/settings.ini");
}

QString themeToString(ThemeMode theme)
{
    switch (theme) {
    case ThemeMode::System:
        return QStringLiteral("system");
    case ThemeMode::Dark:
        return QStringLiteral("dark");
    case ThemeMode::Light:
        return QStringLiteral("light");
    }
    return QStringLiteral("system");
}

ThemeMode themeFromString(const QString &themeName)
{
    const QString normalized = themeName.trimmed().toLower();
    if (normalized == QStringLiteral("dark")) {
        return ThemeMode::Dark;
    }
    if (normalized == QStringLiteral("light")) {
        return ThemeMode::Light;
    }
    return ThemeMode::System;
}

Settings load()
{
    Settings settings;
    QSettings ini(settingsFilePath(), QSettings::IniFormat);
    ini.beginGroup(QStringLiteral("appearance"));
    settings.theme = themeFromString(ini.value(QStringLiteral("theme"), themeToString(settings.theme)).toString());
    settings.language = ini.value(QStringLiteral("language"), settings.language).toString();
    ini.endGroup();
    return settings;
}

void save(const Settings &settings)
{
    QSettings ini(settingsFilePath(), QSettings::IniFormat);
    ini.beginGroup(QStringLiteral("appearance"));
    ini.setValue(QStringLiteral("theme"), themeToString(settings.theme));
    ini.setValue(QStringLiteral("language"), settings.language);
    ini.endGroup();
    ini.sync();
}

void applyTheme(QApplication &application, ThemeMode theme)
{
    DiagramTheme::setTheme(theme);

    if (theme == ThemeMode::System) {
        application.setStyleSheet(QString());
    } else {
        const QString resourcePath = theme == ThemeMode::Dark
            ? QStringLiteral(":/dark/stylesheet.qss")
            : QStringLiteral(":/light/stylesheet.qss");

        QFile file(resourcePath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            application.setStyleSheet(QString());
        } else {
            QTextStream stream(&file);
            application.setStyleSheet(stream.readAll());
        }
    }

    for (QWidget *widget : QApplication::allWidgets()) {
        if (auto *diagramView = qobject_cast<SubstationDiagramView *>(widget)) {
            diagramView->refreshTheme();
        }
        widget->update();
    }
}

bool applyLanguage(QApplication &application, const QString &language, QTranslator &translator)
{
    application.removeTranslator(&translator);

    if (!language.trimmed().startsWith(QStringLiteral("uk"), Qt::CaseInsensitive)) {
        return true;
    }

    if (!translator.load(QStringLiteral(":/translations/app_uk.qm"))) {
        return false;
    }

    application.installTranslator(&translator);
    return true;
}

} // namespace AppSettings

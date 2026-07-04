#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>

class QApplication;

namespace AppSettings {

enum class ThemeMode {
    System,
    Dark,
    Light
};

struct Settings
{
    ThemeMode theme = ThemeMode::System;
    QString language = QStringLiteral("en");
};

QString settingsFilePath();
Settings load();
void save(const Settings &settings);
void applyTheme(QApplication &application, ThemeMode theme);

QString themeToString(ThemeMode theme);
ThemeMode themeFromString(const QString &themeName);

} // namespace AppSettings

#endif // APPSETTINGS_H

#ifndef DIAGRAMTHEME_H
#define DIAGRAMTHEME_H

#include <QColor>

#include "appsettings.h"

namespace DiagramTheme {
enum class ColorRole {
    Background,
    Grid,
    Busbar,
    Branch,
    Accent,
    Selection,
    Border,
    Text,
    NodeFill,
    PanelBackground,
    PanelText
};

void setTheme(AppSettings::ThemeMode theme);
QColor color(ColorRole role);
} // namespace DiagramTheme

#endif // DIAGRAMTHEME_H

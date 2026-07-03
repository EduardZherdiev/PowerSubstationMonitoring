#ifndef DIAGRAMTHEME_H
#define DIAGRAMTHEME_H

#include <QColor>

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

QColor color(ColorRole role);
}

#endif // DIAGRAMTHEME_H

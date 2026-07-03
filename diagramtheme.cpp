#include "diagramtheme.h"

namespace DiagramTheme {
QColor color(ColorRole role)
{
    switch (role) {
    case ColorRole::Background:
        return QColor::fromRgb(0x0B1220);
    case ColorRole::Grid:
        return QColor::fromRgb(255, 255, 255, 12);
    case ColorRole::Busbar:
        return QColor::fromRgb(0x56C2FF);
    case ColorRole::Branch:
        return QColor::fromRgb(0x7A8CA8);
    case ColorRole::Accent:
        return QColor::fromRgb(0xFFB86B);
    case ColorRole::Selection:
        return QColor::fromRgb(0xFF3B30);
    case ColorRole::Border:
        return QColor::fromRgb(0x8DA0B6);
    case ColorRole::Text:
        return QColor::fromRgb(0xFFFFFF);
    case ColorRole::NodeFill:
        return QColor::fromRgb(0x1A2A42);
    case ColorRole::PanelBackground:
        return QColor::fromRgb(0x111A2B);
    case ColorRole::PanelText:
        return QColor::fromRgb(0xE7EEF9);
    }
    return QColor::fromRgb(0xFFFFFF);
}
}

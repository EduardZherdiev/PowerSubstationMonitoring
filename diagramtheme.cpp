#include "diagramtheme.h"

namespace {

AppSettings::ThemeMode currentTheme = AppSettings::ThemeMode::Dark;

QColor darkColor(DiagramTheme::ColorRole role)
{
    switch (role) {
    case DiagramTheme::ColorRole::Background:
        return QColor::fromRgb(0x0B1220);
    case DiagramTheme::ColorRole::Grid:
        return QColor::fromRgb(255, 255, 255, 12);
    case DiagramTheme::ColorRole::Busbar:
        return QColor::fromRgb(0x56C2FF);
    case DiagramTheme::ColorRole::Branch:
        return QColor::fromRgb(0x7A8CA8);
    case DiagramTheme::ColorRole::Accent:
        return QColor::fromRgb(0xFFB86B);
    case DiagramTheme::ColorRole::Selection:
        return QColor::fromRgb(0xFF3B30);
    case DiagramTheme::ColorRole::Border:
        return QColor::fromRgb(0x8DA0B6);
    case DiagramTheme::ColorRole::Text:
        return QColor::fromRgb(0xFFFFFF);
    case DiagramTheme::ColorRole::NodeFill:
        return QColor::fromRgb(0x1A2A42);
    case DiagramTheme::ColorRole::PanelBackground:
        return QColor::fromRgb(0x111A2B);
    case DiagramTheme::ColorRole::PanelText:
        return QColor::fromRgb(0xE7EEF9);
    }
    return QColor::fromRgb(0xFFFFFF);
}

QColor lightColor(DiagramTheme::ColorRole role)
{
    switch (role) {
    case DiagramTheme::ColorRole::Background:
        return QColor::fromRgb(0xF5F7FA);
    case DiagramTheme::ColorRole::Text:
        return QColor::fromRgb(0x17212B);
    case DiagramTheme::ColorRole::Grid:
    case DiagramTheme::ColorRole::Busbar:
    case DiagramTheme::ColorRole::Branch:
    case DiagramTheme::ColorRole::Accent:
    case DiagramTheme::ColorRole::Selection:
    case DiagramTheme::ColorRole::Border:
    case DiagramTheme::ColorRole::NodeFill:
    case DiagramTheme::ColorRole::PanelBackground:
    case DiagramTheme::ColorRole::PanelText:
        return darkColor(role);
    }
    return QColor::fromRgb(0x17212B);
}

QColor themedColor(DiagramTheme::ColorRole role)
{
    if (currentTheme == AppSettings::ThemeMode::Light) {
        return lightColor(role);
    }
    return darkColor(role);
}

} // namespace

namespace DiagramTheme {
void setTheme(AppSettings::ThemeMode theme)
{
    currentTheme = theme;
}

QColor color(ColorRole role)
{
    return themedColor(role);
}
}

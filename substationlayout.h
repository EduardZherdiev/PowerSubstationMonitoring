#ifndef SUBSTATIONLAYOUT_H
#define SUBSTATIONLAYOUT_H

#include "diagramnodeitem.h"
#include "diagramtheme.h"

#include <QMap>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <QVector>

namespace SubstationLayout {

struct NodeSpec
{
    QString id;
    QString title;
    DiagramNodeItem::ShapeType shape = DiagramNodeItem::ShapeType::LineTerminal;
    QString type;
    QString status;
    QString location;
    QString description;
    QMap<QString, QString> parameters;
    QPointF position;
    QSizeF size;
};

struct ConnectionSpec
{
    QString fromId;
    QString toId;
    DiagramTheme::ColorRole colorRole = DiagramTheme::ColorRole::Branch;
    qreal width = 2.0;
};

struct Layout
{
    QVector<NodeSpec> nodes;
    QVector<ConnectionSpec> connections;
};

QString defaultLayoutPath();
bool loadFromFile(const QString &path, Layout *layout, QString *errorMessage = nullptr);
DiagramNodeItem::ShapeType shapeTypeFromString(const QString &shapeName);
DiagramTheme::ColorRole colorRoleFromString(const QString &roleName);

} // namespace SubstationLayout

#endif // SUBSTATIONLAYOUT_H

#include "substationlayout.h"

#include <QCoreApplication>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace SubstationLayout {

namespace {

QString readString(const QJsonObject &object, const char *key, const QString &fallback = QString())
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isString() ? value.toString() : fallback;
}

qreal readReal(const QJsonObject &object, const char *key, qreal fallback = 0.0)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isDouble() ? value.toDouble() : fallback;
}

QPointF readPoint(const QJsonObject &object)
{
    const QJsonObject positionObject = object.value(QStringLiteral("position")).toObject();
    return QPointF(readReal(positionObject, "x"), readReal(positionObject, "y"));
}

QSizeF readSize(const QJsonObject &object)
{
    const QJsonObject sizeObject = object.value(QStringLiteral("size")).toObject();
    return QSizeF(readReal(sizeObject, "width", 120.0), readReal(sizeObject, "height", 90.0));
}

QMap<QString, QString> readParameters(const QJsonObject &object)
{
    QMap<QString, QString> parameters;
    const QJsonObject parametersObject = object.value(QStringLiteral("parameters")).toObject();
    for (auto it = parametersObject.begin(); it != parametersObject.end(); ++it) {
        parameters.insert(it.key(), it.value().toVariant().toString());
    }
    return parameters;
}

} // namespace

QString defaultLayoutPath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/substation_layout.json");
}

DiagramNodeItem::ShapeType shapeTypeFromString(const QString &shapeName)
{
    const QString normalized = shapeName.trimmed().toLower();
    if (normalized == QStringLiteral("busbar")) {
        return DiagramNodeItem::ShapeType::Busbar;
    }
    if (normalized == QStringLiteral("breaker")) {
        return DiagramNodeItem::ShapeType::Breaker;
    }
    if (normalized == QStringLiteral("transformer")) {
        return DiagramNodeItem::ShapeType::Transformer;
    }
    if (normalized == QStringLiteral("station")) {
        return DiagramNodeItem::ShapeType::Station;
    }
    return DiagramNodeItem::ShapeType::LineTerminal;
}

DiagramTheme::ColorRole colorRoleFromString(const QString &roleName)
{
    const QString normalized = roleName.trimmed().toLower();
    if (normalized == QStringLiteral("busbar")) {
        return DiagramTheme::ColorRole::Busbar;
    }
    if (normalized == QStringLiteral("accent")) {
        return DiagramTheme::ColorRole::Accent;
    }
    return DiagramTheme::ColorRole::Branch;
}

bool loadFromFile(const QString &path, Layout *layout, QString *errorMessage)
{
    if (!layout) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("SubstationLayout", "Layout output is null.");
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("SubstationLayout", "Unable to open layout file: %1").arg(path);
        }
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("SubstationLayout", "Invalid layout JSON document.");
        }
        return false;
    }

    const QJsonObject rootObject = document.object();
    const QJsonArray nodesArray = rootObject.value(QStringLiteral("nodes")).toArray();
    const QJsonArray connectionsArray = rootObject.value(QStringLiteral("connections")).toArray();

    Layout parsedLayout;
    parsedLayout.nodes.reserve(nodesArray.size());
    parsedLayout.connections.reserve(connectionsArray.size());

    QJsonObject nodeObject;
    NodeSpec node;
    for (const QJsonValue &nodeValue : nodesArray) {
        nodeObject = nodeValue.toObject();
        node.id = readString(nodeObject, "id");
        node.title = readString(nodeObject, "title", node.id);
        node.shape = shapeTypeFromString(readString(nodeObject, "shape"));
        node.type = readString(nodeObject, "type");
        node.status = readString(nodeObject, "status");
        node.location = readString(nodeObject, "location");
        node.description = readString(nodeObject, "description");
        node.parameters = readParameters(nodeObject);
        node.position = readPoint(nodeObject);
        node.size = readSize(nodeObject);

        if (!node.id.isEmpty()) {
            parsedLayout.nodes.append(node);
        }
    }

    QJsonObject connectionObject;
    ConnectionSpec connection;
    for (const QJsonValue &connectionValue : connectionsArray) {
        connectionObject = connectionValue.toObject();
        connection.fromId = readString(connectionObject, "from");
        connection.toId = readString(connectionObject, "to");
        connection.colorRole = colorRoleFromString(readString(connectionObject, "role"));
        connection.width = readReal(connectionObject, "width", 2.0);

        if (!connection.fromId.isEmpty() && !connection.toId.isEmpty()) {
            parsedLayout.connections.append(connection);
        }
    }

    *layout = parsedLayout;
    return true;
}

} // namespace SubstationLayout

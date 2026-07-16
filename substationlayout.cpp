#include "substationlayout.h"

#include <QCoreApplication>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>

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

namespace {

QString colorRoleToString(DiagramTheme::ColorRole role)
{
    switch (role) {
    case DiagramTheme::ColorRole::Busbar:
        return QStringLiteral("Busbar");
    case DiagramTheme::ColorRole::Accent:
        return QStringLiteral("Accent");
    default:
        return QStringLiteral("Branch");
    }
}

} // namespace

bool loadFromFile(const QString &path, Layout *layout, QString *errorMessage)
{
    if (!layout) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("SubstationLayout",
                                                        "Layout output is null.");
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("SubstationLayout",
                                                        "Unable to open layout file: %1")
                                .arg(path);
        }
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("SubstationLayout",
                                                        "Invalid layout JSON document.");
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

bool saveToFile(const QString &path, const Layout &layout, QString *errorMessage)
{
    QJsonObject rootObject;
    QJsonArray nodesArray;
    for (const NodeSpec &node : layout.nodes) {
        QJsonObject nodeObject;
        nodeObject.insert(QStringLiteral("id"), node.id);
        nodeObject.insert(QStringLiteral("title"), node.title);
        nodeObject.insert(QStringLiteral("shape"), [&node]() {
            switch (node.shape) {
            case DiagramNodeItem::ShapeType::Busbar:
                return QStringLiteral("Busbar");
            case DiagramNodeItem::ShapeType::Breaker:
                return QStringLiteral("Breaker");
            case DiagramNodeItem::ShapeType::Transformer:
                return QStringLiteral("Transformer");
            case DiagramNodeItem::ShapeType::Station:
                return QStringLiteral("Station");
            case DiagramNodeItem::ShapeType::LineTerminal:
                return QStringLiteral("LineTerminal");
            }
            return QStringLiteral("LineTerminal");
        }());
        nodeObject.insert(QStringLiteral("position"),
                          QJsonObject{{QStringLiteral("x"), node.position.x()},
                                      {QStringLiteral("y"), node.position.y()}});
        nodeObject.insert(QStringLiteral("size"),
                          QJsonObject{{QStringLiteral("width"), node.size.width()},
                                      {QStringLiteral("height"), node.size.height()}});
        nodeObject.insert(QStringLiteral("type"), node.type);
        nodeObject.insert(QStringLiteral("status"), node.status);
        nodeObject.insert(QStringLiteral("location"), node.location);
        nodeObject.insert(QStringLiteral("description"), node.description);

        QJsonObject parametersObject;
        for (auto it = node.parameters.cbegin(); it != node.parameters.cend(); ++it) {
            parametersObject.insert(it.key(), it.value());
        }
        nodeObject.insert(QStringLiteral("parameters"), parametersObject);
        nodesArray.append(nodeObject);
    }

    QJsonArray connectionsArray;
    for (const ConnectionSpec &connection : layout.connections) {
        QJsonObject connectionObject;
        connectionObject.insert(QStringLiteral("from"), connection.fromId);
        connectionObject.insert(QStringLiteral("to"), connection.toId);
        connectionObject.insert(QStringLiteral("role"), colorRoleToString(connection.colorRole));
        connectionObject.insert(QStringLiteral("width"), connection.width);
        connectionsArray.append(connectionObject);
    }
    rootObject.insert(QStringLiteral("nodes"), nodesArray);
    rootObject.insert(QStringLiteral("connections"), connectionsArray);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("SubstationLayout",
                                                        "Unable to save layout file: %1")
                                .arg(path);
        }
        return false;
    }

    file.write(QJsonDocument(rootObject).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("SubstationLayout",
                                                        "Unable to finalize layout file: %1")
                                .arg(path);
        }
        return false;
    }
    return true;
}

} // namespace SubstationLayout

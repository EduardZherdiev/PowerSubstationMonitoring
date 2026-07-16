#include "telemetrylayoutmapper.h"

#include "sensortelemetry.h"
#include "substationlayout.h"

#include <QCoreApplication>

namespace {

QString formatKv(double value) {
    return QCoreApplication::translate("TelemetryLayoutMapper", "%1 kV").arg(QString::number(value, 'f', 1));
}

QString formatA(double value) {
    return QCoreApplication::translate("TelemetryLayoutMapper", "%1 A").arg(QString::number(value, 'f', 1));
}

QString formatC(double value) {
    return QCoreApplication::translate("TelemetryLayoutMapper", "%1 C").arg(QString::number(value, 'f', 1));
}

void removeLiveMeasurements(SubstationLayout::NodeSpec* node) {
    if (!node) {
        return;
    }

    node->parameters.remove(QStringLiteral("Voltage"));
    node->parameters.remove(QStringLiteral("Current"));
    node->parameters.remove(QStringLiteral("Temperature"));
    node->parameters.remove(QStringLiteral("Load"));
}

} // namespace

namespace TelemetryLayoutMapper {

void apply(SubstationLayout::Layout* layout, const SensorSnapshot& snapshot) {
    if (!layout) {
        return;
    }

    for (SubstationLayout::NodeSpec& node : layout->nodes) {
        removeLiveMeasurements(&node);

        if (node.id == QStringLiteral("Line-1")) {
            node.parameters[QStringLiteral("Voltage")] = formatKv(snapshot.sourceVoltageKv);
            node.parameters[QStringLiteral("Current")] = formatA(snapshot.sourceCurrentA);
            node.status = QStringLiteral("Ready");
        } else if (node.id == QStringLiteral("CB-1")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Closed") : QStringLiteral("Open");
        } else if (node.id == QStringLiteral("Bus-1")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Energized") : QStringLiteral("De-energized");
            node.parameters[QStringLiteral("Voltage")] =
                snapshot.breakerClosed ? formatKv(snapshot.sourceVoltageKv) : formatKv(0.0);
        } else if (node.id == QStringLiteral("TR-1")) {
            node.parameters[QStringLiteral("Temperature")] = formatC(snapshot.transformerTemperatureC);
            node.parameters[QStringLiteral("Load")] =
                QCoreApplication::translate("TelemetryLayoutMapper", "%1%")
                    .arg(QString::number(snapshot.transformerLoadPercent, 'f', 1));
            node.parameters[QStringLiteral("Voltage")] = snapshot.breakerClosed ? formatKv(12.0) : formatKv(0.0);
            node.parameters[QStringLiteral("Current")] =
                snapshot.breakerClosed ? formatA(snapshot.sourceCurrentA) : formatA(0.0);
            node.status = snapshot.breakerClosed ? QStringLiteral("Loading") : QStringLiteral("Cooling");
        } else if (node.id == QStringLiteral("Bus-2")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Energized") : QStringLiteral("De-energized");
            node.parameters[QStringLiteral("Voltage")] = snapshot.breakerClosed ? formatKv(12.0) : formatKv(0.0);
        } else if (node.id == QStringLiteral("Line-2")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Ready") : QStringLiteral("De-energized");
            node.parameters[QStringLiteral("Voltage")] = snapshot.breakerClosed ? formatKv(12.0) : formatKv(0.0);
            node.parameters[QStringLiteral("Current")] =
                snapshot.breakerClosed ? formatA(snapshot.sourceCurrentA) : formatA(0.0);
        }
    }
}

} // namespace TelemetryLayoutMapper

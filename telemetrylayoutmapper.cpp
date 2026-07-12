#include "telemetrylayoutmapper.h"

#include "sensortelemetry.h"
#include "substationlayout.h"

namespace {

QString formatKv(double value)
{
    return QStringLiteral("%1 kV").arg(QString::number(value, 'f', 1));
}

QString formatA(double value)
{
    return QStringLiteral("%1 A").arg(QString::number(value, 'f', 1));
}

QString formatC(double value)
{
    return QStringLiteral("%1 C").arg(QString::number(value, 'f', 1));
}

} // namespace

namespace TelemetryLayoutMapper {

void apply(SubstationLayout::Layout *layout, const SensorSnapshot &snapshot)
{
    if (!layout) {
        return;
    }

    for (SubstationLayout::NodeSpec &node : layout->nodes) {
        if (node.id == QStringLiteral("Line-1")) {
            node.parameters[QStringLiteral("Voltage")] = formatKv(snapshot.sourceVoltageKv);
            node.parameters[QStringLiteral("Current")] = formatA(snapshot.sourceCurrentA);
            node.status = QStringLiteral("Ready");
        } else if (node.id == QStringLiteral("CB-1")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Closed") : QStringLiteral("Open");
        } else if (node.id == QStringLiteral("Bus-1")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Energized") : QStringLiteral("De-energized");
            node.parameters[QStringLiteral("Voltage")] = snapshot.breakerClosed ? formatKv(snapshot.sourceVoltageKv) : QStringLiteral("0.0 kV");
            node.parameters[QStringLiteral("Current")] = snapshot.breakerClosed ? formatA(snapshot.sourceCurrentA) : QStringLiteral("0.0 A");
        } else if (node.id == QStringLiteral("TR-1")) {
            node.parameters[QStringLiteral("Temperature")] = formatC(snapshot.transformerTemperatureC);
            node.parameters[QStringLiteral("Load")] = QStringLiteral("%1%").arg(QString::number(snapshot.transformerLoadPercent, 'f', 1));
            node.parameters[QStringLiteral("Voltage")] = snapshot.breakerClosed ? QStringLiteral("12.0 kV") : QStringLiteral("0.0 kV");
            node.parameters[QStringLiteral("Current")] = snapshot.breakerClosed ? formatA(snapshot.sourceCurrentA) : QStringLiteral("0.0 A");
            node.status = snapshot.breakerClosed ? QStringLiteral("Loading") : QStringLiteral("Cooling");
        } else if (node.id == QStringLiteral("Bus-2")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Energized") : QStringLiteral("De-energized");
            node.parameters[QStringLiteral("Voltage")] = snapshot.breakerClosed ? QStringLiteral("12.0 kV") : QStringLiteral("0.0 kV");
            node.parameters[QStringLiteral("Current")] = snapshot.breakerClosed ? formatA(snapshot.sourceCurrentA) : QStringLiteral("0.0 A");
        } else if (node.id == QStringLiteral("Line-2")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Ready") : QStringLiteral("De-energized");
            node.parameters[QStringLiteral("Voltage")] = snapshot.breakerClosed ? QStringLiteral("12.0 kV") : QStringLiteral("0.0 kV");
            node.parameters[QStringLiteral("Current")] = snapshot.breakerClosed ? formatA(snapshot.sourceCurrentA) : QStringLiteral("0.0 A");
        }
    }
}

} // namespace TelemetryLayoutMapper

#include "sensortelemetry.h"

#include <QCoreApplication>
#include <QRandomGenerator>
#include <QtMath>

namespace {

double boundedNoise(double magnitude)
{
    return (QRandomGenerator::global()->generateDouble() * 2.0 - 1.0) * magnitude;
}

double smoothWave(double phase, double scale, double offset = 0.0)
{
    return offset + qSin(phase) * scale;
}

} // namespace

SensorTelemetrySimulator::SensorTelemetrySimulator()
    : m_phase(0.0)
    , m_breakerClosed(true)
{}

bool SensorTelemetrySimulator::tryReadSnapshot(SensorSnapshot *result)
{
    if (!result) {
        return false;
    }

    m_phase += 0.18;

    SensorSnapshot snapshot;
    snapshot.sourceVoltageKv = 330.0 + smoothWave(m_phase * 0.45, 2.8) + boundedNoise(0.8);
    snapshot.sourceCurrentA = 540.0 + smoothWave(m_phase * 0.63 + 0.4, 18.0) + boundedNoise(4.0);
    snapshot.transformerLoadPercent = 72.0 + smoothWave(m_phase * 0.30, 5.5) + boundedNoise(1.2);
    snapshot.transformerTemperatureC = 68.0 + smoothWave(m_phase * 0.27 + 1.0, 4.0)
                                       + boundedNoise(0.7);
    m_breakerClosed = true;
    snapshot.breakerClosed = true;
    snapshot.temperatureBySensor.insert(QStringLiteral("TS-TR-1"), snapshot.transformerTemperatureC);

    if (snapshot.sourceVoltageKv < 320.0) {
        snapshot.notes.append(
            QCoreApplication::translate("SensorTelemetry",
                                        "Source voltage dropped below expected range"));
    }
    if (snapshot.transformerTemperatureC > 80.0) {
        snapshot.notes.append(
            QCoreApplication::translate("SensorTelemetry", "Transformer temperature warning"));
    }

    *result = snapshot;
    return true;
}

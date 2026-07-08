#ifndef SENSORTELEMETRY_H
#define SENSORTELEMETRY_H

#include <QMap>
#include <QStringList>

struct SensorSnapshot
{
    double sourceVoltageKv = 330.0;
    double sourceCurrentA = 540.0;
    double transformerTemperatureC = 68.0;
    double transformerLoadPercent = 72.0;
    bool breakerClosed = true;
    QMap<QString, double> temperatureBySensor;
    QStringList notes;
};

class SensorTelemetrySimulator
{
public:
    SensorTelemetrySimulator();

    SensorSnapshot nextSnapshot();

private:
    double m_phase;
    bool m_breakerClosed;
};

#endif // SENSORTELEMETRY_H
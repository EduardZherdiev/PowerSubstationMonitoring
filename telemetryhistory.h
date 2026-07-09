#ifndef TELEMETRYHISTORY_H
#define TELEMETRYHISTORY_H

#include <QDateTime>
#include <QVector>

struct TelemetrySample
{
    QDateTime timestamp;
    double value = 0.0;
};

namespace TelemetryHistory {

enum class SeriesKind {
    Voltage,
    Current,
    Temperature
};

QVector<TelemetrySample> generateSeries(SeriesKind kind, const QDateTime &endTime, int windowSeconds);

} // namespace TelemetryHistory

#endif // TELEMETRYHISTORY_H
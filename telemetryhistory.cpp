#include "telemetryhistory.h"

#include <QtMath>

namespace {

int sampleStepSeconds(int windowSeconds)
{
    if (windowSeconds <= 60) {
        return 1;
    }
    if (windowSeconds <= 3600) {
        return 5;
    }
    if (windowSeconds <= 24 * 3600) {
        return 60;
    }
    return 15 * 60;
}

double seedOffsetForDate(const QDateTime &timestamp)
{
    const int daySeed = timestamp.date().dayOfYear();
    return (daySeed % 17) * 0.12;
}

double seriesValue(TelemetryHistory::SeriesKind kind, const QDateTime &timestamp)
{
    const double seconds = static_cast<double>(timestamp.toSecsSinceEpoch());
    const double dayOffset = seedOffsetForDate(timestamp);

    switch (kind) {
    case TelemetryHistory::SeriesKind::Voltage:
        return 330.0 + qSin(seconds / 240.0 + dayOffset) * 2.2 + qSin(seconds / 45.0) * 0.3 + qCos(seconds / 600.0) * 0.4;
    case TelemetryHistory::SeriesKind::Current:
        return 540.0 + qSin(seconds / 160.0 + 0.6 + dayOffset) * 20.0 + qCos(seconds / 52.0) * 2.6;
    case TelemetryHistory::SeriesKind::Temperature:
        return 68.0 + qSin(seconds / 220.0 + 1.2 + dayOffset) * 4.5 + qCos(seconds / 90.0) * 0.7;
    }

    return 0.0;
}

} // namespace

namespace TelemetryHistory {

QVector<TelemetrySample> generateSeries(SeriesKind kind, const QDateTime &endTime, int windowSeconds)
{
    QVector<TelemetrySample> samples;
    if (!endTime.isValid() || windowSeconds <= 0) {
        return samples;
    }

    const int stepSeconds = sampleStepSeconds(windowSeconds);
    const QDateTime startTime = endTime.addSecs(-windowSeconds);
    const int sampleCount = qMax(2, windowSeconds / qMax(1, stepSeconds) + 1);
    samples.reserve(sampleCount);

    QDateTime timestamp = startTime;
    while (timestamp <= endTime) {
        samples.append(TelemetrySample{timestamp, seriesValue(kind, timestamp)});
        timestamp = timestamp.addSecs(stepSeconds);
    }

    if (samples.isEmpty() || samples.last().timestamp < endTime) {
        samples.append(TelemetrySample{endTime, seriesValue(kind, endTime)});
    }

    return samples;
}

} // namespace TelemetryHistory
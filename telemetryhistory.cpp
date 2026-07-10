#include "telemetryhistory.h"

#include <QHash>
#include <QtMath>

namespace {

QHash<int, QVector<TelemetrySample>> &seriesStorage()
{
    static QHash<int, QVector<TelemetrySample>> storage;
    return storage;
}

int seriesKey(TelemetryHistory::SeriesKind kind)
{
    return static_cast<int>(kind);
}

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

QVector<TelemetrySample> generatedSeries(TelemetryHistory::SeriesKind kind,
                                         const QDateTime &endTime,
                                         int windowSeconds)
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

} // namespace

namespace TelemetryHistory {

void appendSample(SeriesKind kind, const TelemetrySample &sample)
{
    if (!sample.timestamp.isValid()) {
        return;
    }

    QVector<TelemetrySample> &samples = seriesStorage()[seriesKey(kind)];
    samples.append(sample);

    const QDateTime cutoff = sample.timestamp.addDays(-31);
    while (!samples.isEmpty() && samples.first().timestamp < cutoff) {
        samples.removeFirst();
    }
}

QVector<TelemetrySample> series(SeriesKind kind, const QDateTime &endTime, int windowSeconds)
{
    QVector<TelemetrySample> filtered = generatedSeries(kind, endTime, windowSeconds);
    if (!endTime.isValid() || windowSeconds <= 0) {
        return filtered;
    }

    const QVector<TelemetrySample> samples = seriesStorage().value(seriesKey(kind));
    if (filtered.isEmpty() || samples.isEmpty()) {
        return filtered;
    }

    const QDateTime startTime = endTime.addSecs(-windowSeconds);
    const int stepSeconds = sampleStepSeconds(windowSeconds);
    QHash<qint64, int> indexByBucket;
    indexByBucket.reserve(filtered.size());

    for (int i = 0; i < filtered.size(); ++i) {
        const qint64 offset = startTime.secsTo(filtered.at(i).timestamp);
        indexByBucket.insert(offset / qMax(1, stepSeconds), i);
    }

    for (const TelemetrySample &sample : samples) {
        if (sample.timestamp >= startTime && sample.timestamp <= endTime) {
            const qint64 offset = startTime.secsTo(sample.timestamp);
            const qint64 bucket = offset / qMax(1, stepSeconds);
            const auto it = indexByBucket.constFind(bucket);
            if (it != indexByBucket.constEnd()) {
                filtered[it.value()] = sample;
            } else {
                filtered.append(sample);
            }
        }
    }

    return filtered;
}

} // namespace TelemetryHistory

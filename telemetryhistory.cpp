#include "telemetryhistory.h"

#include <QHash>
#include <QtMath>
#include <qtimezone.h>

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

QDateTime alignTimestamp(const QDateTime &timestamp, int stepSeconds)
{
    if (!timestamp.isValid()) {
        return timestamp;
    }

    const qint64 step = qMax(1, stepSeconds);
    const qint64 secs = timestamp.toSecsSinceEpoch();
    const qint64 alignedSecs = (secs / step) * step;
    return QDateTime::fromSecsSinceEpoch(alignedSecs, timestamp.timeZone());
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
    const QDateTime alignedEndTime = alignTimestamp(endTime, stepSeconds);
    const QDateTime startTime = alignedEndTime.addSecs(-windowSeconds);
    const int sampleCount = qMax(2, windowSeconds / qMax(1, stepSeconds) + 1);
    samples.reserve(sampleCount);

    QDateTime timestamp = startTime;
    while (timestamp <= alignedEndTime) {
        samples.append(TelemetrySample{timestamp, seriesValue(kind, timestamp)});
        timestamp = timestamp.addSecs(stepSeconds);
    }

    if (samples.isEmpty() || samples.last().timestamp < alignedEndTime) {
        samples.append(TelemetrySample{alignedEndTime, seriesValue(kind, alignedEndTime)});
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

    const int stepSeconds = sampleStepSeconds(windowSeconds);
    const QDateTime alignedEndTime = alignTimestamp(endTime, stepSeconds);
    const QDateTime startTime = alignedEndTime.addSecs(-windowSeconds);
    QHash<qint64, int> indexByTimestamp;
    indexByTimestamp.reserve(filtered.size());

    for (int i = 0; i < filtered.size(); ++i) {
        indexByTimestamp.insert(filtered.at(i).timestamp.toSecsSinceEpoch(), i);
    }

    for (const TelemetrySample &sample : samples) {
        if (sample.timestamp >= startTime && sample.timestamp <= endTime) {
            const QDateTime alignedSampleTime = alignTimestamp(sample.timestamp, stepSeconds);
            const auto it = indexByTimestamp.constFind(alignedSampleTime.toSecsSinceEpoch());
            if (it != indexByTimestamp.constEnd()) {
                filtered[it.value()] = TelemetrySample{alignedSampleTime, sample.value};
            } else {
                filtered.append(TelemetrySample{alignedSampleTime, sample.value});
            }
        }
    }

    return filtered;
}

} // namespace TelemetryHistory

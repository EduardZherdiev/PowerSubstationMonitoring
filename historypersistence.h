#ifndef HISTORYPERSISTENCE_H
#define HISTORYPERSISTENCE_H

#include "eventlogger.h"
#include "telemetryhistory.h"

namespace HistoryPersistence {

QVector<EventRecord> loadEvents();
bool appendEvent(const EventRecord& record);

QVector<TelemetrySample> loadTelemetry(TelemetryHistory::SeriesKind kind);
bool appendTelemetry(TelemetryHistory::SeriesKind kind, const TelemetrySample& sample);

QString storageDirectoryPath();

} // namespace HistoryPersistence

#endif // HISTORYPERSISTENCE_H

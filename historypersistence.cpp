#include "historypersistence.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

namespace {

constexpr int retentionDays = 31;

QString eventsFilePath() {
    return QDir(HistoryPersistence::storageDirectoryPath()).filePath(QStringLiteral("events.jsonl"));
}

QString telemetryFilePath(TelemetryHistory::SeriesKind kind) {
    QString name;
    switch (kind) {
    case TelemetryHistory::SeriesKind::Voltage:
        name = QStringLiteral("voltage.jsonl");
        break;
    case TelemetryHistory::SeriesKind::Current:
        name = QStringLiteral("current.jsonl");
        break;
    case TelemetryHistory::SeriesKind::Temperature:
        name = QStringLiteral("temperature.jsonl");
        break;
    }
    return QDir(HistoryPersistence::storageDirectoryPath()).filePath(name);
}

bool appendJsonLine(const QString& filePath, const QJsonObject& object) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return false;
    }
    const QByteArray line = QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
    return file.write(line) == line.size();
}

QVector<QJsonObject> loadObjects(const QString& filePath) {
    QVector<QJsonObject> objects;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return objects;
    }

    while (!file.atEnd()) {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(file.readLine().trimmed(), &error);
        if (error.error == QJsonParseError::NoError && document.isObject()) {
            objects.append(document.object());
        }
    }
    return objects;
}

void rewriteObjects(const QString& filePath, const QVector<QJsonObject>& objects) {
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    for (const QJsonObject& object : objects) {
        file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
        file.write("\n");
    }
    file.commit();
}

QDateTime timestampFromObject(const QJsonObject& object) {
    return QDateTime::fromString(object.value(QStringLiteral("timestamp")).toString(), Qt::ISODateWithMs);
}

} // namespace

namespace HistoryPersistence {

QString storageDirectoryPath() {
    const QString path =
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("history"));
    QDir().mkpath(path);
    return path;
}

QVector<EventRecord> loadEvents() {
    QVector<EventRecord> records;
    QVector<QJsonObject> retainedObjects;
    const QDateTime cutoff = QDateTime::currentDateTime().addDays(-retentionDays);

    for (const QJsonObject& object : loadObjects(eventsFilePath())) {
        const QDateTime timestamp = timestampFromObject(object);
        const int levelValue = object.value(QStringLiteral("level")).toInt(-1);
        if (!timestamp.isValid() || timestamp < cutoff || levelValue < static_cast<int>(EventLevel::Info) ||
            levelValue > static_cast<int>(EventLevel::Critical)) {
            continue;
        }

        records.append(EventRecord{timestamp, static_cast<EventLevel>(levelValue),
                                   object.value(QStringLiteral("source")).toString(),
                                   object.value(QStringLiteral("message")).toString()});
        retainedObjects.append(object);
    }
    std::sort(records.begin(), records.end(),
              [](const EventRecord& left, const EventRecord& right) { return left.timestamp < right.timestamp; });
    rewriteObjects(eventsFilePath(), retainedObjects);
    return records;
}

bool appendEvent(const EventRecord& record) {
    return appendJsonLine(eventsFilePath(),
                          QJsonObject{{QStringLiteral("timestamp"), record.timestamp.toString(Qt::ISODateWithMs)},
                                      {QStringLiteral("level"), static_cast<int>(record.level)},
                                      {QStringLiteral("source"), record.source},
                                      {QStringLiteral("message"), record.message}});
}

QVector<TelemetrySample> loadTelemetry(TelemetryHistory::SeriesKind kind) {
    QVector<TelemetrySample> samples;
    QVector<QJsonObject> retainedObjects;
    const QDateTime cutoff = QDateTime::currentDateTime().addDays(-retentionDays);
    const QString filePath = telemetryFilePath(kind);

    for (const QJsonObject& object : loadObjects(filePath)) {
        const QDateTime timestamp = timestampFromObject(object);
        const QJsonValue value = object.value(QStringLiteral("value"));
        if (!timestamp.isValid() || timestamp < cutoff || !value.isDouble()) {
            continue;
        }
        samples.append(TelemetrySample{timestamp, value.toDouble()});
        retainedObjects.append(object);
    }
    rewriteObjects(filePath, retainedObjects);
    return samples;
}

bool appendTelemetry(TelemetryHistory::SeriesKind kind, const TelemetrySample& sample) {
    // Five-second persistence keeps month-long history compact while live data remains one-second resolution.
    if (sample.timestamp.toSecsSinceEpoch() % 5 != 0) {
        return true;
    }
    return appendJsonLine(telemetryFilePath(kind),
                          QJsonObject{{QStringLiteral("timestamp"), sample.timestamp.toString(Qt::ISODateWithMs)},
                                      {QStringLiteral("value"), sample.value}});
}

} // namespace HistoryPersistence

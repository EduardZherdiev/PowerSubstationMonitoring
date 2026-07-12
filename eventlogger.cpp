#include "eventlogger.h"
#include "historypersistence.h"

#include <QApplication>

EventLogger *EventLogger::instance(QObject *parent)
{
    static EventLogger *logger = nullptr;
    if (!logger) {
        logger = new EventLogger(parent ? parent : qApp);
    }
    return logger;
}

EventLogger::EventLogger(QObject *parent)
    : QObject{parent}
    , m_records(HistoryPersistence::loadEvents())
{}

void EventLogger::info(const QString &source, const QString &message)
{
    appendRecord(EventLevel::Info, source, message);
    emit eventLogged(EventLevel::Info, source, message);
}

void EventLogger::warning(const QString &source, const QString &message)
{
    appendRecord(EventLevel::Warning, source, message);
    emit eventLogged(EventLevel::Warning, source, message);
}

void EventLogger::critical(const QString &source, const QString &message)
{
    appendRecord(EventLevel::Critical, source, message);
    emit eventLogged(EventLevel::Critical, source, message);
}

QVector<EventRecord> EventLogger::records(const QDateTime &from,
                                          const QDateTime &to,
                                          bool warningsAndCriticalOnly) const
{
    QVector<EventRecord> filtered;
    for (const EventRecord &record : m_records) {
        if (from.isValid() && record.timestamp < from) {
            continue;
        }
        if (to.isValid() && record.timestamp > to) {
            continue;
        }
        if (warningsAndCriticalOnly && record.level == EventLevel::Info) {
            continue;
        }
        filtered.append(record);
    }
    return filtered;
}

void EventLogger::appendRecord(EventLevel level, const QString &source, const QString &message)
{
    const EventRecord record{QDateTime::currentDateTime(), level, source, message};
    m_records.append(record);
    HistoryPersistence::appendEvent(record);
}

void logInfo(const QString &source, const QString &message)
{
    EventLogger::instance()->info(source, message);
}

void logWarning(const QString &source, const QString &message)
{
    EventLogger::instance()->warning(source, message);
}

void logCritical(const QString &source, const QString &message)
{
    EventLogger::instance()->critical(source, message);
}

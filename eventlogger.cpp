#include "eventlogger.h"

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
{}

void EventLogger::info(const QString &source, const QString &message)
{
    emit eventLogged(EventLevel::Info, source, message);
}

void EventLogger::warning(const QString &source, const QString &message)
{
    emit eventLogged(EventLevel::Warning, source, message);
}

void EventLogger::critical(const QString &source, const QString &message)
{
    emit eventLogged(EventLevel::Critical, source, message);
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

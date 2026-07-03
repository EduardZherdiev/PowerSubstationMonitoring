#ifndef EVENTLOGGER_H
#define EVENTLOGGER_H

#include <QObject>
#include <QString>

enum class EventLevel {
    Info,
    Warning,
    Critical
};

class EventLogger : public QObject
{
    Q_OBJECT
public:
    static EventLogger *instance(QObject *parent = nullptr);

    void info(const QString &source, const QString &message);
    void warning(const QString &source, const QString &message);
    void critical(const QString &source, const QString &message);

signals:
    void eventLogged(const EventLevel &level, const QString &source, const QString &message);

private:
    explicit EventLogger(QObject *parent = nullptr);
    Q_DISABLE_COPY_MOVE(EventLogger)
};

void logInfo(const QString &source, const QString &message);
void logWarning(const QString &source, const QString &message);
void logCritical(const QString &source, const QString &message);

#endif // EVENTLOGGER_H

#ifndef EVENTLOGGER_H
#define EVENTLOGGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>

enum class EventLevel {
    Info,
    Warning,
    Critical
};

struct EventRecord
{
    QDateTime timestamp;
    EventLevel level = EventLevel::Info;
    QString source;
    QString message;
};

class EventLogger : public QObject
{
    Q_OBJECT
public:
    static EventLogger *instance(QObject *parent = nullptr);

    void info(const QString &source, const QString &message);
    void warning(const QString &source, const QString &message);
    void critical(const QString &source, const QString &message);
    QVector<EventRecord> records(const QDateTime &from, const QDateTime &to, bool warningsAndCriticalOnly = true) const;

signals:
    void eventLogged(const EventLevel &level, const QString &source, const QString &message);

private:
    explicit EventLogger(QObject *parent = nullptr);
    Q_DISABLE_COPY_MOVE(EventLogger)

    void appendRecord(EventLevel level, const QString &source, const QString &message);

    QVector<EventRecord> m_records;
};

void logInfo(const QString &source, const QString &message);
void logWarning(const QString &source, const QString &message);
void logCritical(const QString &source, const QString &message);

#endif // EVENTLOGGER_H

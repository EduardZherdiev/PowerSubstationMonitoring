#ifndef TELEMETRYSERVICE_H
#define TELEMETRYSERVICE_H

#include "sensortelemetry.h"

#include <QObject>

#include <memory>

class QTimer;

class TelemetryService : public QObject
{
    Q_OBJECT

public:
    enum class ConnectionState { Disconnected, Connecting, Connected };
    Q_ENUM(ConnectionState)

    explicit TelemetryService(std::unique_ptr<ITelemetrySource> source, QObject *parent = nullptr);

    void start(int intervalMs = 1000);
    void stop();
    void setBreakerClosed(bool closed);
    void setManualTemperature(double temperatureC);

    ConnectionState connectionState() const;
    bool breakerClosed() const;
    bool manualTemperatureActive() const;
    double displayedTemperature() const;

signals:
    void snapshotReady(const SensorSnapshot &snapshot);
    void connectionStateChanged(TelemetryService::ConnectionState state);

private:
    void setConnectionState(ConnectionState state);
    void poll();
    SensorSnapshot applyControls(const SensorSnapshot &snapshot);

    std::unique_ptr<ITelemetrySource> m_source;
    QTimer *m_timer;
    ConnectionState m_connectionState;
    SensorSnapshot m_lastSnapshot;
    bool m_hasLastSnapshot;
    bool m_breakerClosed;
    bool m_manualTemperatureActive;
    double m_manualTemperature;
    double m_temperatureRecoveryPhase;
};

#endif // TELEMETRYSERVICE_H

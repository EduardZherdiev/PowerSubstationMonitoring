#include "telemetryservice.h"

#include <QTimer>
#include <QtMath>

namespace {

double cooledTemperature(double previousTemperature)
{
    constexpr double ambientTemperature = 34.0;
    return qMax(ambientTemperature, previousTemperature - 1.8);
}

double smoothApproach(double currentValue, double targetValue, double maximumStep)
{
    if (qAbs(targetValue - currentValue) <= maximumStep) {
        return targetValue;
    }
    return currentValue + (targetValue > currentValue ? maximumStep : -maximumStep);
}

} // namespace

TelemetryService::TelemetryService(std::unique_ptr<ITelemetrySource> source, QObject *parent)
    : QObject(parent)
    , m_source(std::move(source))
    , m_timer(new QTimer(this))
    , m_connectionState(ConnectionState::Disconnected)
    , m_hasLastSnapshot(false)
    , m_breakerClosed(true)
    , m_manualTemperatureActive(false)
    , m_manualTemperature(68.0)
    , m_temperatureRecoveryPhase(0.0)
{
    connect(m_timer, &QTimer::timeout, this, &TelemetryService::poll);
}

void TelemetryService::start(int intervalMs)
{
    if (!m_source) {
        setConnectionState(ConnectionState::Disconnected);
        return;
    }

    setConnectionState(ConnectionState::Connecting);
    m_timer->start(qMax(100, intervalMs));
    poll();
}

void TelemetryService::stop()
{
    m_timer->stop();
    setConnectionState(ConnectionState::Disconnected);
}

void TelemetryService::setBreakerClosed(bool closed)
{
    if (m_breakerClosed == closed) {
        return;
    }
    m_breakerClosed = closed;
    poll();
}

void TelemetryService::setManualTemperature(double temperatureC)
{
    m_manualTemperature = temperatureC;
    m_manualTemperatureActive = true;
    m_temperatureRecoveryPhase = 0.0;
    poll();
}

TelemetryService::ConnectionState TelemetryService::connectionState() const
{
    return m_connectionState;
}

bool TelemetryService::breakerClosed() const
{
    return m_breakerClosed;
}

bool TelemetryService::manualTemperatureActive() const
{
    return m_manualTemperatureActive;
}

double TelemetryService::displayedTemperature() const
{
    return m_manualTemperatureActive ? m_manualTemperature : m_lastSnapshot.transformerTemperatureC;
}

void TelemetryService::setConnectionState(ConnectionState state)
{
    if (m_connectionState == state) {
        return;
    }
    m_connectionState = state;
    emit connectionStateChanged(state);
}

void TelemetryService::poll()
{
    if (!m_source) {
        setConnectionState(ConnectionState::Disconnected);
        return;
    }

    SensorSnapshot sourceSnapshot;
    if (!m_source->tryReadSnapshot(&sourceSnapshot)) {
        setConnectionState(ConnectionState::Disconnected);
        return;
    }

    setConnectionState(ConnectionState::Connected);
    SensorSnapshot snapshot = applyControls(sourceSnapshot);
    m_lastSnapshot = snapshot;
    m_hasLastSnapshot = true;
    emit snapshotReady(snapshot);
}

SensorSnapshot TelemetryService::applyControls(const SensorSnapshot &sourceSnapshot)
{
    SensorSnapshot snapshot = sourceSnapshot;
    snapshot.breakerClosed = m_breakerClosed;

    if (!snapshot.breakerClosed) {
        snapshot.sourceCurrentA = 0.0;
        snapshot.transformerLoadPercent = 0.0;
        const double referenceTemperature = m_hasLastSnapshot
            ? m_lastSnapshot.transformerTemperatureC
            : snapshot.transformerTemperatureC;
        snapshot.transformerTemperatureC = cooledTemperature(referenceTemperature);
        snapshot.temperatureBySensor.insert(QStringLiteral("TS-TR-1"), snapshot.transformerTemperatureC);
    }

    if (m_manualTemperatureActive) {
        const double targetTemperature = snapshot.transformerTemperatureC;
        m_temperatureRecoveryPhase += 0.55;
        const double distanceToTarget = qAbs(m_manualTemperature - targetTemperature);
        const double wobbleAmplitude = qMin(0.45, distanceToTarget * 0.18);
        const double wobbleTarget = targetTemperature + qSin(m_temperatureRecoveryPhase) * wobbleAmplitude;
        m_manualTemperature = smoothApproach(m_manualTemperature, wobbleTarget, 0.8);
        snapshot.transformerTemperatureC = m_manualTemperature;
        snapshot.temperatureBySensor.insert(QStringLiteral("TS-TR-1"), snapshot.transformerTemperatureC);

        if (qAbs(m_manualTemperature - targetTemperature) < 0.15) {
            m_manualTemperature = targetTemperature;
            m_manualTemperatureActive = false;
            m_temperatureRecoveryPhase = 0.0;
        }
    }

    return snapshot;
}

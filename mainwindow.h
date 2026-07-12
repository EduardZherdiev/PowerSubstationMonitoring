#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QMainWindow>

#include "telemetryservice.h"

class QResizeEvent;
class Equipment;
class EquipmentTreeModel;
class SubstationDiagramView;
class QGraphicsScene;
class QColor;
class QModelIndex;
enum class EventLevel;
namespace SubstationLayout {
struct Layout;
}

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupModelAndViews();
    void configureParameterPanel();
    void connectInteractions();
    void loadSubstationLayout();
    void startTelemetryMonitoring();
    void processSnapshot(const SensorSnapshot &snapshot);
    void updateConnectionState(TelemetryService::ConnectionState state);
    void setupMonitoringCharts();
    void refreshMonitoringCharts();
    void rememberCurrentSelection(const QModelIndex &index);
    void restoreSelection();
    void displayEquipment(Equipment *equipment, bool fromUserAction);
    void appendEvent(EventLevel level, const QString &source, const QString &message,
                     const QDateTime &timestamp = QDateTime());
    Equipment *equipmentFromTreeIndex(const QModelIndex &index) const;
    Equipment *findEquipmentByName(const QString &name) const;
    int selectedWindowSeconds() const;
    QDateTime selectedEndTime() const;
    void emitTemperatureAlerts(const SensorSnapshot &snapshot);
    void updateBreakerControls(Equipment *equipment);
    void updateTemperatureControls(Equipment *equipment);

    Ui::MainWindow *ui;
    EquipmentTreeModel *equipmentModel;
    SubstationDiagramView *diagramView;
    TelemetryService *m_telemetryService;
    SubstationLayout::Layout *m_baseLayout;
    SubstationLayout::Layout *m_liveLayout;
    QGraphicsScene *m_voltageScene;
    QGraphicsScene *m_currentScene;
    QGraphicsScene *m_temperatureScene;
    QString m_selectedEquipmentName;
    SensorSnapshot m_lastSnapshot;
    bool m_hasLastSnapshot;
};
#endif // MAINWINDOW_H

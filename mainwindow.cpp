#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "equipment.h"
#include "equipmenttreemodel.h"
#include "diagramtheme.h"
#include "powerflowcalculator.h"
#include "sensortelemetry.h"
#include "monitoringchart.h"
#include "telemetryhistory.h"
#include "substationdiagramview.h"
#include "substationlayout.h"
#include "aboutdialog.h"
#include "eventlogger.h"
#include "settingsdialog.h"

#include <QHeaderView>
#include <QDateTime>
#include <QGraphicsScene>
#include <QPalette>
#include <QItemSelectionModel>
#include <QList>
#include <QStringList>
#include <QSplitter>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QStatusBar>
#include <QAbstractItemView>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDir>
#include <QRandomGenerator>

namespace {

QString formatKv(double value)
{
    return QStringLiteral("%1 kV").arg(QString::number(value, 'f', 1));
}

QString formatA(double value)
{
    return QStringLiteral("%1 A").arg(QString::number(value, 'f', 1));
}

QString formatC(double value)
{
    return QStringLiteral("%1 C").arg(QString::number(value, 'f', 1));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , equipmentModel(new EquipmentTreeModel(this))
    , diagramView(nullptr)
    , m_sensorSimulator(new SensorTelemetrySimulator())
    , m_telemetryTimer(new QTimer(this))
    , m_baseLayout(new SubstationLayout::Layout())
    , m_liveLayout(new SubstationLayout::Layout())
    , m_voltageScene(MonitoringChart::createScene(this))
    , m_currentScene(MonitoringChart::createScene(this))
    , m_temperatureScene(MonitoringChart::createScene(this))
    , m_hasLastSnapshot(false)
{
    ui->setupUi(this);
    diagramView = ui->graphicsView;
    setupMonitoringCharts();
    ui->period->setChecked(true);
    ui->dateEdit->setEnabled(false);
    ui->periodComboBox->setCurrentIndex(0);

    QPalette windowPalette = palette();
    windowPalette.setColor(QPalette::Window, DiagramTheme::color(DiagramTheme::ColorRole::Background));
    windowPalette.setColor(QPalette::Base, DiagramTheme::color(DiagramTheme::ColorRole::PanelBackground));
    windowPalette.setColor(QPalette::Text, DiagramTheme::color(DiagramTheme::ColorRole::PanelText));
    windowPalette.setColor(QPalette::WindowText, DiagramTheme::color(DiagramTheme::ColorRole::PanelText));
    setPalette(windowPalette);
    ui->centralwidget->setAutoFillBackground(true);
    ui->centralwidget->setPalette(windowPalette);
    ui->parameterPanel->setAutoFillBackground(true);
    ui->parameterPanel->setPalette(windowPalette);

    ui->equipmentTreeView->setModel(equipmentModel);
    ui->equipmentTreeView->header()->setStretchLastSection(true);
    ui->equipmentTreeView->expandToDepth(1);

    if (ui->topSplitter) {
        ui->topSplitter->setStretchFactor(0, 1);
        ui->topSplitter->setStretchFactor(1, 3);
        ui->topSplitter->setStretchFactor(2, 1);
        ui->topSplitter->setSizes(QList<int>{260, 780, 260});
    }

    configureParameterPanel();
    connectInteractions();
    loadSubstationLayout();
    setupModelAndViews();
    startTelemetryMonitoring();
}

MainWindow::~MainWindow()
{
    delete m_baseLayout;
    delete m_liveLayout;
    delete m_sensorSimulator;
    delete ui;
}

void MainWindow::setupModelAndViews()
{
    const QModelIndex initialIndex = equipmentModel->index(0, 0);
    if (initialIndex.isValid()) {
        QSignalBlocker blocker(ui->equipmentTreeView->selectionModel());
        ui->equipmentTreeView->setCurrentIndex(initialIndex);
    }

    if (const Equipment *equipment = equipmentModel->equipmentForIndex(initialIndex)) {
        m_selectedEquipmentName = equipment->name();
    }

    displayEquipment(equipmentModel->equipmentForIndex(initialIndex), false);
    statusBar()->showMessage("Monitoring dashboard ready", 3000);
}

void MainWindow::configureParameterPanel()
{
    ui->parametersTable->horizontalHeader()->setStretchLastSection(true);
    ui->parametersTable->verticalHeader()->hide();
    ui->parametersTable->setColumnWidth(0, 130);
}

void MainWindow::connectInteractions()
{
    connect(EventLogger::instance(), &EventLogger::eventLogged, this, [this](EventLevel level, const QString &source, const QString &message) {
        appendEvent(level, source, message);
    });

    connect(ui->equipmentTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current, const QModelIndex &) {
                rememberCurrentSelection(current);
                displayEquipment(equipmentFromTreeIndex(current), true);
            });

    connect(diagramView, &SubstationDiagramView::equipmentActivated, this, [this](const QString &equipmentKey) {
        Equipment *selectedEquipment = findEquipmentByName(equipmentKey);
        displayEquipment(selectedEquipment, true);
        QModelIndex equipmentIndex = equipmentModel->indexForEquipment(selectedEquipment);
        if (equipmentIndex.isValid()) {
            QSignalBlocker blocker(ui->equipmentTreeView->selectionModel());
            ui->equipmentTreeView->setCurrentIndex(equipmentIndex);
        }
    });
    connect(ui->actionAbout, &QAction::triggered, this,[this]() {
        AboutDialog aboutDialog(this);
        aboutDialog.exec();
    });
    connect(ui->actionSettings, &QAction::triggered, this,[this]() {
        SettingsDialog settingsDialog(this);
        settingsDialog.exec();
    });

    connect(ui->periodComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        refreshMonitoringCharts();
    });

    connect(ui->period, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->dateEdit->setEnabled(false);
            refreshMonitoringCharts();
        }
    });

    connect(ui->day, &QRadioButton::toggled, this, [this](bool checked) {
        ui->dateEdit->setEnabled(checked);
        if (checked) {
            refreshMonitoringCharts();
        }
    });

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, [this](const QDate &) {
        if (ui->day->isChecked()) {
            refreshMonitoringCharts();
        }
    });

}

void MainWindow::loadSubstationLayout()
{
    SubstationLayout::Layout layout;
    QString errorMessage;
    const QStringList candidatePaths = {
        SubstationLayout::defaultLayoutPath(),
        QDir::current().filePath(QStringLiteral("substation_layout.json")),
        QDir::current().filePath(QStringLiteral("data/substation_layout.json"))
    };

    bool loaded = false;
    for (const QString &layoutPath : candidatePaths) {
        if (SubstationLayout::loadFromFile(layoutPath, &layout, &errorMessage)) {
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        statusBar()->showMessage(errorMessage, 5000);
        return;
    }

    *m_baseLayout = layout;
    *m_liveLayout = layout;

    refreshFromTelemetry();
}

void MainWindow::setupMonitoringCharts()
{
    ui->voltage->setScene(m_voltageScene);
    ui->current->setScene(m_currentScene);
    ui->tempreture->setScene(m_temperatureScene);
    ui->voltage->setRenderHint(QPainter::Antialiasing, true);
    ui->current->setRenderHint(QPainter::Antialiasing, true);
    ui->tempreture->setRenderHint(QPainter::Antialiasing, true);
    ui->voltage->setAlignment(Qt::AlignCenter);
    ui->current->setAlignment(Qt::AlignCenter);
    ui->tempreture->setAlignment(Qt::AlignCenter);
}

void MainWindow::startTelemetryMonitoring()
{
    connect(m_telemetryTimer, &QTimer::timeout, this, &MainWindow::refreshFromTelemetry);
    m_telemetryTimer->start(1000);
    refreshFromTelemetry();
}

int MainWindow::selectedWindowSeconds() const
{
    if (ui->day && ui->day->isChecked()) {
        return 24 * 3600;
    }

    if (!ui->periodComboBox) {
        return 60;
    }

    const QString text = ui->periodComboBox->currentText();
    if (text == QStringLiteral("Last minute")) {
        return 60;
    }
    if (text == QStringLiteral("Last hour")) {
        return 3600;
    }
    return 24 * 3600;
}

QDateTime MainWindow::selectedEndTime() const
{
    if (ui->day && ui->day->isChecked()) {
        return QDateTime(ui->dateEdit->date(), QTime(23, 59, 59));
    }

    return QDateTime::currentDateTime();
}

void MainWindow::emitTemperatureAlerts(const SensorSnapshot &snapshot)
{
    constexpr double warningThreshold = 75.0;
    constexpr double criticalThreshold = 85.0;

    if (!m_hasLastSnapshot) {
        if (snapshot.transformerTemperatureC >= criticalThreshold) {
            logCritical(QStringLiteral("Telemetry"), QStringLiteral("Transformer temperature critical: %1").arg(formatC(snapshot.transformerTemperatureC)));
        } else if (snapshot.transformerTemperatureC >= warningThreshold) {
            logWarning(QStringLiteral("Telemetry"), QStringLiteral("Transformer temperature warning: %1").arg(formatC(snapshot.transformerTemperatureC)));
        }
        return;
    }

    const double previous = m_lastSnapshot.transformerTemperatureC;
    const double current = snapshot.transformerTemperatureC;

    if (previous < warningThreshold && current >= warningThreshold && current < criticalThreshold) {
        logWarning(QStringLiteral("Telemetry"), QStringLiteral("Transformer temperature warning: %1").arg(formatC(current)));
    }

    if (previous < criticalThreshold && current >= criticalThreshold) {
        logCritical(QStringLiteral("Telemetry"), QStringLiteral("Transformer temperature critical: %1").arg(formatC(current)));
    }
}

void MainWindow::refreshFromTelemetry()
{
    if (!m_baseLayout || m_baseLayout->nodes.isEmpty()) {
        return;
    }

    const SensorSnapshot snapshot = m_sensorSimulator->nextSnapshot();
    if (m_hasLastSnapshot) {
        if (m_lastSnapshot.breakerClosed != snapshot.breakerClosed) {
            logWarning(QStringLiteral("Telemetry"), snapshot.breakerClosed ? QStringLiteral("CB-1 closed") : QStringLiteral("CB-1 opened"));
        }

        if (snapshot.sourceVoltageKv > m_lastSnapshot.sourceVoltageKv + 2.0 || snapshot.sourceVoltageKv < m_lastSnapshot.sourceVoltageKv - 2.0) {
            logInfo(QStringLiteral("Telemetry"), QStringLiteral("Source voltage changed to %1").arg(formatKv(snapshot.sourceVoltageKv)));
        }

        if (snapshot.transformerTemperatureC > m_lastSnapshot.transformerTemperatureC + 1.5 || snapshot.transformerTemperatureC < m_lastSnapshot.transformerTemperatureC - 1.5) {
            logInfo(QStringLiteral("Telemetry"), QStringLiteral("Transformer temperature changed to %1").arg(formatC(snapshot.transformerTemperatureC)));
        }
    }

    emitTemperatureAlerts(snapshot);

    *m_liveLayout = *m_baseLayout;
    applySnapshotToLayout(m_liveLayout, snapshot);
    PowerFlowCalculator::annotateLayout(m_liveLayout, snapshot.temperatureBySensor);

    equipmentModel->setLayout(*m_liveLayout);
    diagramView->setLayout(*m_liveLayout);
    if (!m_selectedEquipmentName.isEmpty()) {
        diagramView->selectEquipment(m_selectedEquipmentName);
    }
    restoreSelection();

    if (!snapshot.notes.isEmpty()) {
        for (const QString &note : snapshot.notes) {
            logInfo(QStringLiteral("Telemetry"), note);
        }
    }

    refreshMonitoringCharts();

    statusBar()->showMessage(QStringLiteral("Live: %1 | %2 | %3")
                                 .arg(formatKv(snapshot.sourceVoltageKv),
                                      formatA(snapshot.sourceCurrentA),
                                      formatC(snapshot.transformerTemperatureC)),
                             1500);
    m_lastSnapshot = snapshot;
    m_hasLastSnapshot = true;
}

void MainWindow::refreshMonitoringCharts()
{
    const int windowSeconds = selectedWindowSeconds();
    const QDateTime endTime = selectedEndTime();

    MonitoringChart::render(m_voltageScene,
                            TelemetryHistory::generateSeries(TelemetryHistory::SeriesKind::Voltage, endTime, windowSeconds),
                            QStringLiteral("Voltage"),
                            QStringLiteral("kV"),
                            QColor(0x56, 0xC2, 0xFF),
                            DiagramTheme::color(DiagramTheme::ColorRole::PanelText),
                            windowSeconds);
    MonitoringChart::render(m_currentScene,
                            TelemetryHistory::generateSeries(TelemetryHistory::SeriesKind::Current, endTime, windowSeconds),
                            QStringLiteral("Current"),
                            QStringLiteral("A"),
                            QColor(0xFF, 0xB8, 0x6B),
                            DiagramTheme::color(DiagramTheme::ColorRole::PanelText),
                            windowSeconds);
    MonitoringChart::render(m_temperatureScene,
                            TelemetryHistory::generateSeries(TelemetryHistory::SeriesKind::Temperature, endTime, windowSeconds),
                            QStringLiteral("Temperature"),
                            QStringLiteral("C"),
                            QColor(0xD9, 0x2D, 0x20),
                            DiagramTheme::color(DiagramTheme::ColorRole::PanelText),
                            windowSeconds);
}

void MainWindow::applySnapshotToLayout(SubstationLayout::Layout *layout, const SensorSnapshot &snapshot)
{
    if (!layout) {
        return;
    }

    for (SubstationLayout::NodeSpec &node : layout->nodes) {
        if (node.id == QStringLiteral("Line-1")) {
            node.parameters[QStringLiteral("Voltage")] = formatKv(snapshot.sourceVoltageKv);
            node.parameters[QStringLiteral("Current")] = formatA(snapshot.sourceCurrentA);
            node.status = QStringLiteral("Ready");
        } else if (node.id == QStringLiteral("CB-1")) {
            node.status = snapshot.breakerClosed ? QStringLiteral("Closed") : QStringLiteral("Open");
        } else if (node.id == QStringLiteral("TR-1")) {
            node.parameters[QStringLiteral("Temperature")] = formatC(snapshot.transformerTemperatureC);
            node.parameters[QStringLiteral("Load")] = QStringLiteral("%1%").arg(QString::number(snapshot.transformerLoadPercent, 'f', 1));
        }
    }
}

void MainWindow::rememberCurrentSelection(const QModelIndex &index)
{
    if (Equipment *equipment = equipmentFromTreeIndex(index)) {
        m_selectedEquipmentName = equipment->name();
    }
}

void MainWindow::restoreSelection()
{
    if (m_selectedEquipmentName.isEmpty()) {
        return;
    }

    Equipment *selectedEquipment = equipmentModel->equipmentByName(m_selectedEquipmentName);
    if (!selectedEquipment) {
        return;
    }

    const QModelIndex equipmentIndex = equipmentModel->indexForEquipment(selectedEquipment);
    if (equipmentIndex.isValid()) {
        QSignalBlocker blocker(ui->equipmentTreeView->selectionModel());
        ui->equipmentTreeView->setCurrentIndex(equipmentIndex);
    }

    displayEquipment(selectedEquipment, false);
}

void MainWindow::displayEquipment(Equipment *equipment, bool fromUserAction)
{
    if (!equipment) {
        return;
    }

    m_selectedEquipmentName = equipment->name();

    ui->nameValueLabel->setText(equipment->name());
    ui->typeValueLabel->setText(equipment->type());
    ui->statusValueLabel->setText(equipment->status());
    ui->locationValueLabel->setText(equipment->location().isEmpty() ? "-" : equipment->location());
    ui->descriptionValueLabel->setText(equipment->description().isEmpty() ? "-" : equipment->description());

    const auto parameters = equipment->parameters();
    ui->parametersTable->clearContents();
    ui->parametersTable->setRowCount(parameters.size());
    int row = 0;
    for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it, ++row) {
        ui->parametersTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        ui->parametersTable->setItem(row, 1, new QTableWidgetItem(it.value()));
    }

    if (diagramView) {
        diagramView->selectEquipment(equipment->name());
    }

    if (fromUserAction) {
        logInfo("User Action", QString("Selected equipment: %1").arg(equipment->name()));
    }
}

void MainWindow::appendEvent(EventLevel level, const QString &source, const QString &message)
{
    if (!ui->eventLogTable) {
        return;
    }

    int row = ui->eventLogTable->rowCount();
    ui->eventLogTable->insertRow(row);

    QTableWidgetItem *levelItem = new QTableWidgetItem();
    switch (level) {
        case EventLevel::Info:
            levelItem->setText("Info");
            break;
        case EventLevel::Warning:
            levelItem->setText("Warning");
            levelItem->setForeground(Qt::yellow);
            break;
        case EventLevel::Critical:
            levelItem->setText("Critical");
            levelItem->setForeground(Qt::red);
            break;
    }
    QTableWidgetItem *timeItem = new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss"));
    ui->eventLogTable->setItem(row, 1, levelItem);
    ui->eventLogTable->setItem(row, 0, timeItem);
    ui->eventLogTable->setItem(row, 2, new QTableWidgetItem(source));
    ui->eventLogTable->setItem(row, 3, new QTableWidgetItem(message));
}

Equipment *MainWindow::equipmentFromTreeIndex(const QModelIndex &index) const
{
    return equipmentModel->equipmentForIndex(index);
}

Equipment *MainWindow::findEquipmentByName(const QString &name) const
{
    return equipmentModel->equipmentByName(name);
}

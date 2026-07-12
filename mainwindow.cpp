#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "equipment.h"
#include "equipmenttreemodel.h"
#include "diagramtheme.h"
#include "powerflowcalculator.h"
#include "sensortelemetry.h"
#include "monitoringchart.h"
#include "telemetryhistory.h"
#include "telemetryservice.h"
#include "telemetrylayoutmapper.h"
#include "substationdiagramview.h"
#include "substationlayout.h"
#include "aboutdialog.h"
#include "eventlogger.h"
#include "reportdialog.h"
#include "settingsdialog.h"

#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QGraphicsScene>
#include <QPalette>
#include <QItemSelectionModel>
#include <QList>
#include <QStringList>
#include <QSplitter>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QShortcut>
#include <QStatusBar>
#include <QAbstractItemView>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDir>
#include <memory>

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

bool equipmentSupportsTemperatureControl(const Equipment *equipment)
{
    if (!equipment) {
        return false;
    }

    const QMap<QString, QString> &parameters = equipment->parameters();
    return parameters.contains(QStringLiteral("Temperature"))
        || parameters.contains(QStringLiteral("Temperature Sensor"))
        || parameters.contains(QStringLiteral("Rated Temperature"));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , equipmentModel(new EquipmentTreeModel(this))
    , diagramView(nullptr)
    , m_telemetryService(new TelemetryService(std::make_unique<SensorTelemetrySimulator>(), this))
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
    ui->breakerControlLabel->setVisible(false);
    ui->breakerStateCombo->setVisible(false);
    ui->breakerStateCombo->setCurrentText(QStringLiteral("Closed"));
    ui->temperatureControlLabel->setVisible(false);
    ui->temperatureSpinBox->setVisible(false);
    ui->applyTemperatureButton->setVisible(false);
    ui->temperatureSpinBox->setSuffix(QStringLiteral(" C"));
    ui->temperatureSpinBox->setRange(-40.0, 150.0);
    ui->temperatureSpinBox->setDecimals(1);
    ui->temperatureSpinBox->setSingleStep(0.5);

    auto *copyShortcut = new QShortcut(QKeySequence::Copy, ui->eventLogTable);
    connect(copyShortcut, &QShortcut::activated, this, [this]() {
        if (!ui->eventLogTable) {
            return;
        }

        const int row = ui->eventLogTable->currentRow();
        if (row < 0) {
            return;
        }

        QStringList cells;
        cells.reserve(ui->eventLogTable->columnCount());
        for (int column = 0; column < ui->eventLogTable->columnCount(); ++column) {
            QTableWidgetItem *item = ui->eventLogTable->item(row, column);
            cells.append(item ? item->text() : QString());
        }
        QGuiApplication::clipboard()->setText(cells.join(QStringLiteral("\t")));
    });
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
    connect(ui->actionReport, &QAction::triggered, this, [this]() {
        ReportDialog reportDialog(this);
        reportDialog.exec();
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

    connect(ui->breakerStateCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        const bool shouldBeClosed = text != QStringLiteral("Open");
        if (m_telemetryService->breakerClosed() == shouldBeClosed) {
            return;
        }

        logInfo(QStringLiteral("User Action"),
                QStringLiteral("Breaker CB-1 set to %1").arg(shouldBeClosed ? QStringLiteral("Closed")
                                                                            : QStringLiteral("Open")));
        m_telemetryService->setBreakerClosed(shouldBeClosed);
    });
    connect(ui->applyTemperatureButton, &QPushButton::clicked, this, [this]() {
        const double temperature = ui->temperatureSpinBox->value();
        logInfo(QStringLiteral("User Action"),
                QStringLiteral("Transformer temperature set to %1").arg(formatC(temperature)));
        m_telemetryService->setManualTemperature(temperature);
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

    if (m_hasLastSnapshot) {
        processSnapshot(m_lastSnapshot);
    }
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
    connect(m_telemetryService, &TelemetryService::snapshotReady,
            this, &MainWindow::processSnapshot);
    connect(m_telemetryService, &TelemetryService::connectionStateChanged,
            this, &MainWindow::updateConnectionState);
    updateConnectionState(m_telemetryService->connectionState());
    m_telemetryService->start(1000);
}

void MainWindow::updateConnectionState(TelemetryService::ConnectionState state)
{
    QString text;
    QString backgroundColor;
    QString foregroundColor;
    switch (state) {
    case TelemetryService::ConnectionState::Connected:
        text = QStringLiteral("Connection: Connected");
        backgroundColor = QStringLiteral("#1f6f43");
        foregroundColor = QStringLiteral("#ffffff");
        break;
    case TelemetryService::ConnectionState::Connecting:
        text = QStringLiteral("Connection: Connecting...");
        backgroundColor = QStringLiteral("#8a651b");
        foregroundColor = QStringLiteral("#ffffff");
        break;
    case TelemetryService::ConnectionState::Disconnected:
        text = QStringLiteral("Connection: Disconnected");
        backgroundColor = QStringLiteral("#8b2f36");
        foregroundColor = QStringLiteral("#ffffff");
        break;
    }

    ui->connectionStatusLabel->setText(text);
    ui->connectionStatusLabel->setStyleSheet(
        QStringLiteral("QLabel { border-radius: 6px; padding: 5px 10px; font-weight: 600; "
                       "background-color: %1; color: %2; }")
            .arg(backgroundColor, foregroundColor));
    ui->connectionStatusLabel->setAccessibleName(text);
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
    const double current = snapshot.transformerTemperatureC;
    if (current >= criticalThreshold) {
        logCritical(QStringLiteral("Telemetry"), QStringLiteral("Transformer temperature critical: %1").arg(formatC(current)));
    } else if (current >= warningThreshold) {
        logWarning(QStringLiteral("Telemetry"), QStringLiteral("Transformer temperature warning: %1").arg(formatC(current)));
    }
}

void MainWindow::processSnapshot(const SensorSnapshot &adjustedSnapshot)
{
    if (!m_baseLayout || m_baseLayout->nodes.isEmpty()) {
        return;
    }

    const QDateTime sampleTime = QDateTime::currentDateTime();
    const double monitoredVoltageKv = adjustedSnapshot.breakerClosed ? adjustedSnapshot.sourceVoltageKv : 0.0;
    const double monitoredCurrentA = adjustedSnapshot.breakerClosed ? adjustedSnapshot.sourceCurrentA : 0.0;
    TelemetryHistory::appendSample(TelemetryHistory::SeriesKind::Voltage, TelemetrySample{sampleTime, monitoredVoltageKv});
    TelemetryHistory::appendSample(TelemetryHistory::SeriesKind::Current, TelemetrySample{sampleTime, monitoredCurrentA});
    TelemetryHistory::appendSample(TelemetryHistory::SeriesKind::Temperature,
                                   TelemetrySample{sampleTime, adjustedSnapshot.transformerTemperatureC});

    if (m_hasLastSnapshot) {
        if (m_lastSnapshot.breakerClosed != adjustedSnapshot.breakerClosed) {
            logWarning(QStringLiteral("Telemetry"), adjustedSnapshot.breakerClosed ? QStringLiteral("CB-1 closed") : QStringLiteral("CB-1 opened"));
        }

        if (adjustedSnapshot.sourceVoltageKv > m_lastSnapshot.sourceVoltageKv + 2.0 || adjustedSnapshot.sourceVoltageKv < m_lastSnapshot.sourceVoltageKv - 2.0) {
            logInfo(QStringLiteral("Telemetry"), QStringLiteral("Source voltage changed to %1").arg(formatKv(adjustedSnapshot.sourceVoltageKv)));
        }

        if (adjustedSnapshot.transformerTemperatureC > m_lastSnapshot.transformerTemperatureC + 1.5 || adjustedSnapshot.transformerTemperatureC < m_lastSnapshot.transformerTemperatureC - 1.5) {
            logInfo(QStringLiteral("Telemetry"), QStringLiteral("Transformer temperature changed to %1").arg(formatC(adjustedSnapshot.transformerTemperatureC)));
        }
    }

    emitTemperatureAlerts(adjustedSnapshot);

    *m_liveLayout = *m_baseLayout;
    TelemetryLayoutMapper::apply(m_liveLayout, adjustedSnapshot);
    PowerFlowCalculator::annotateLayout(m_liveLayout, adjustedSnapshot.temperatureBySensor);

    equipmentModel->updateLayout(*m_liveLayout);
    if (diagramView->substationLayout().nodes.isEmpty()) {
        diagramView->setLayout(*m_liveLayout);
    }
    if (m_selectedEquipmentName.isEmpty()) {
        setupModelAndViews();
    } else {
        diagramView->selectEquipment(m_selectedEquipmentName);
        restoreSelection();
    }

    if (!adjustedSnapshot.notes.isEmpty()) {
        for (const QString &note : adjustedSnapshot.notes) {
            logInfo(QStringLiteral("Telemetry"), note);
        }
    }

    refreshMonitoringCharts();

    statusBar()->showMessage(QStringLiteral("Live: %1 | %2 | %3")
                                 .arg(formatKv(adjustedSnapshot.sourceVoltageKv),
                                      formatA(adjustedSnapshot.sourceCurrentA),
                                      formatC(adjustedSnapshot.transformerTemperatureC)),
                             1500);
    m_lastSnapshot = adjustedSnapshot;
    m_hasLastSnapshot = true;
}

void MainWindow::refreshMonitoringCharts()
{
    const int windowSeconds = selectedWindowSeconds();
    const QDateTime endTime = selectedEndTime();

    MonitoringChart::render(m_voltageScene,
                            TelemetryHistory::series(TelemetryHistory::SeriesKind::Voltage, endTime, windowSeconds),
                            QStringLiteral("Voltage"),
                            QStringLiteral("kV"),
                            QColor(0x56, 0xC2, 0xFF),
                            DiagramTheme::color(DiagramTheme::ColorRole::PanelText),
                            windowSeconds);
    MonitoringChart::render(m_currentScene,
                            TelemetryHistory::series(TelemetryHistory::SeriesKind::Current, endTime, windowSeconds),
                            QStringLiteral("Current"),
                            QStringLiteral("A"),
                            QColor(0xFF, 0xB8, 0x6B),
                            DiagramTheme::color(DiagramTheme::ColorRole::PanelText),
                            windowSeconds);
    MonitoringChart::render(m_temperatureScene,
                            TelemetryHistory::series(TelemetryHistory::SeriesKind::Temperature, endTime, windowSeconds),
                            QStringLiteral("Temperature"),
                            QStringLiteral("C"),
                            QColor(0xD9, 0x2D, 0x20),
                            DiagramTheme::color(DiagramTheme::ColorRole::PanelText),
                            windowSeconds);
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
    updateBreakerControls(equipment);
    updateTemperatureControls(equipment);

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

void MainWindow::updateBreakerControls(Equipment *equipment)
{
    const bool isBreaker = equipment && equipment->name() == QStringLiteral("CB-1");
    ui->breakerControlLabel->setVisible(isBreaker);
    ui->breakerStateCombo->setVisible(isBreaker);

    if (!isBreaker) {
        return;
    }

    const QSignalBlocker blocker(ui->breakerStateCombo);
    ui->breakerStateCombo->setCurrentText(m_telemetryService->breakerClosed() ? QStringLiteral("Closed")
                                                                              : QStringLiteral("Open"));
}

void MainWindow::updateTemperatureControls(Equipment *equipment)
{
    const bool hasTemperatureControl = equipmentSupportsTemperatureControl(equipment);
    ui->temperatureControlLabel->setVisible(hasTemperatureControl);
    ui->temperatureSpinBox->setVisible(hasTemperatureControl);
    ui->applyTemperatureButton->setVisible(hasTemperatureControl);

    if (!hasTemperatureControl) {
        return;
    }

    if (ui->temperatureSpinBox->hasFocus()) {
        return;
    }

    const QSignalBlocker blocker(ui->temperatureSpinBox);
    const double displayedTemperature = m_hasLastSnapshot
        ? m_telemetryService->displayedTemperature()
        : 68.0;
    ui->temperatureSpinBox->setValue(displayedTemperature);
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

    constexpr int maximumVisibleEvents = 1000;
    while (ui->eventLogTable->rowCount() > maximumVisibleEvents) {
        ui->eventLogTable->removeRow(0);
    }
    row = ui->eventLogTable->rowCount() - 1;
    ui->eventLogTable->selectRow(row);
    ui->eventLogTable->scrollToBottom();
}

Equipment *MainWindow::equipmentFromTreeIndex(const QModelIndex &index) const
{
    return equipmentModel->equipmentForIndex(index);
}

Equipment *MainWindow::findEquipmentByName(const QString &name) const
{
    return equipmentModel->equipmentByName(name);
}

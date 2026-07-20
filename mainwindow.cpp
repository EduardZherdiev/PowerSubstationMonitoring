#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "diagramtheme.h"
#include "equipment.h"
#include "equipmenttreemodel.h"
#include "eventlogger.h"
#include "monitoringchart.h"
#include "powerflowcalculator.h"
#include "reportdialog.h"
#include "sensortelemetry.h"
#include "settingsdialog.h"
#include "substationdiagramview.h"
#include "substationlayout.h"
#include "telemetryhistory.h"
#include "telemetrylayoutmapper.h"
#include "telemetryservice.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QList>
#include <QPair>
#include <QPalette>
#include <QResizeEvent>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVector>
#include <memory>

namespace {

QString formatKv(double value) {
    return QCoreApplication::translate("MainWindow", "%1 kV").arg(QString::number(value, 'f', 1));
}

QString formatA(double value) {
    return QCoreApplication::translate("MainWindow", "%1 A").arg(QString::number(value, 'f', 1));
}

QString formatC(double value) {
    return QCoreApplication::translate("MainWindow", "%1 C").arg(QString::number(value, 'f', 1));
}

QString translatedEquipmentType(const QString& type) {
    if (type == QStringLiteral("Transmission Line")) {
        return QCoreApplication::translate("Equipment", "Transmission Line");
    }
    if (type == QStringLiteral("Circuit Breaker")) {
        return QCoreApplication::translate("Equipment", "Circuit Breaker");
    }
    if (type == QStringLiteral("Transformer")) {
        return QCoreApplication::translate("Equipment", "Transformer");
    }
    if (type == QStringLiteral("Busbar")) {
        return QCoreApplication::translate("Equipment", "Busbar");
    }
    return type;
}

QString translatedEquipmentStatus(const QString& status) {
    if (status == QStringLiteral("Ready")) {
        return QCoreApplication::translate("Equipment", "Ready");
    }
    if (status == QStringLiteral("Closed")) {
        return QCoreApplication::translate("Equipment", "Closed");
    }
    if (status == QStringLiteral("Open")) {
        return QCoreApplication::translate("Equipment", "Open");
    }
    if (status == QStringLiteral("Energized")) {
        return QCoreApplication::translate("Equipment", "Energized");
    }
    if (status == QStringLiteral("De-energized")) {
        return QCoreApplication::translate("Equipment", "De-energized");
    }
    if (status == QStringLiteral("Loading")) {
        return QCoreApplication::translate("Equipment", "Loading");
    }
    if (status == QStringLiteral("Cooling")) {
        return QCoreApplication::translate("Equipment", "Cooling");
    }
    return status;
}

QString translatedParameterName(const QString& name) {
    if (name == QStringLiteral("Voltage")) {
        return QCoreApplication::translate("EquipmentParameter", "Voltage");
    }
    if (name == QStringLiteral("Current")) {
        return QCoreApplication::translate("EquipmentParameter", "Current");
    }
    if (name == QStringLiteral("Temperature")) {
        return QCoreApplication::translate("EquipmentParameter", "Temperature");
    }
    if (name == QStringLiteral("Temperature Sensor")) {
        return QCoreApplication::translate("EquipmentParameter", "Temperature Sensor");
    }
    if (name == QStringLiteral("Rated Temperature")) {
        return QCoreApplication::translate("EquipmentParameter", "Rated Temperature");
    }
    if (name == QStringLiteral("Rated Current")) {
        return QCoreApplication::translate("EquipmentParameter", "Rated Current");
    }
    if (name == QStringLiteral("Trip Count")) {
        return QCoreApplication::translate("EquipmentParameter", "Trip Count");
    }
    if (name == QStringLiteral("Transformation Ratio")) {
        return QCoreApplication::translate("EquipmentParameter", "Transformation Ratio");
    }
    if (name == QStringLiteral("MVA")) {
        return QCoreApplication::translate("EquipmentParameter", "MVA");
    }
    if (name == QStringLiteral("Load")) {
        return QCoreApplication::translate("EquipmentParameter", "Load");
    }
    return name;
}

bool equipmentSupportsTemperatureControl(const Equipment* equipment) {
    if (!equipment) {
        return false;
    }

    const QMap<QString, QString>& parameters = equipment->parameters();
    return equipment->type().contains(QStringLiteral("Transformer"), Qt::CaseInsensitive) ||
           equipment->name().startsWith(QStringLiteral("TR-"), Qt::CaseInsensitive) ||
           parameters.contains(QStringLiteral("Temperature Sensor")) ||
           parameters.contains(QStringLiteral("Rated Temperature"));
}

bool equipmentSupportsVoltage(const Equipment* equipment) {
    if (!equipment) {
        return false;
    }

    const QString type = equipment->type();
    return type.contains(QStringLiteral("Line"), Qt::CaseInsensitive) ||
           type.contains(QStringLiteral("Transformer"), Qt::CaseInsensitive) ||
           type.contains(QStringLiteral("Busbar"), Qt::CaseInsensitive);
}

bool equipmentSupportsCurrent(const Equipment* equipment) {
    if (!equipment) {
        return false;
    }

    const QString type = equipment->type();
    return type.contains(QStringLiteral("Line"), Qt::CaseInsensitive) ||
           type.contains(QStringLiteral("Transformer"), Qt::CaseInsensitive);
}

bool isInternalParameter(const QString& key) {
    return key == QStringLiteral("Calculated Voltage") || key == QStringLiteral("Calculated Current") ||
           key == QStringLiteral("Energized") || key == QStringLiteral("Upstream") ||
           key == QStringLiteral("Flow Note");
}

bool shouldDisplayParameter(const Equipment* equipment, const QString& key) {
    if (isInternalParameter(key)) {
        return false;
    }

    if (key == QStringLiteral("Temperature") || key == QStringLiteral("Temperature Sensor") ||
        key == QStringLiteral("Rated Temperature")) {
        return equipmentSupportsTemperatureControl(equipment);
    }

    if (key == QStringLiteral("Voltage")) {
        return equipmentSupportsVoltage(equipment);
    }

    if (key == QStringLiteral("Current")) {
        return equipmentSupportsCurrent(equipment);
    }

    return true;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), equipmentModel(new EquipmentTreeModel(this)), diagramView(nullptr),
      m_telemetryService(new TelemetryService(std::make_unique<SensorTelemetrySimulator>(), this)),
      m_baseLayout(new SubstationLayout::Layout()), m_liveLayout(new SubstationLayout::Layout()),
      m_voltageScene(MonitoringChart::createScene(this)), m_currentScene(MonitoringChart::createScene(this)),
      m_temperatureScene(MonitoringChart::createScene(this)), m_layoutFileLabel(nullptr), m_hasLastSnapshot(false) {
    ui->setupUi(this);
    m_layoutFileLabel = new QLabel(this);
    m_layoutFileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusBar()->addPermanentWidget(m_layoutFileLabel, 1);
    updateLayoutFileLabel();
    diagramView = ui->graphicsView;
    setupMonitoringCharts();
    ui->period->setChecked(true);
    ui->dateEdit->setEnabled(false);
    ui->periodComboBox->setCurrentIndex(0);

    applyThemePalette();

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

MainWindow::~MainWindow() {
    delete m_baseLayout;
    delete m_liveLayout;
    delete ui;
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        equipmentModel->retranslate();
        ui->temperatureSpinBox->setSuffix(tr(" C"));

        updateConnectionState(m_telemetryService->connectionState());
        if (Equipment* equipment = findEquipmentByName(m_selectedEquipmentName)) {
            displayEquipment(equipment, false);
        }
        refreshMonitoringCharts();
    }
    if (event->type() == QEvent::StyleChange) {
        applyThemePalette();
        refreshMonitoringCharts();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    updateMonitoringChartLayout();
    // Recalculate scene dimensions after the tab and its scroll area settle.
    refreshMonitoringCharts();
}

void MainWindow::applyThemePalette() {
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
}

void MainWindow::setupModelAndViews() {
    const QModelIndex initialIndex = equipmentModel->index(0, 0);
    if (initialIndex.isValid()) {
        QSignalBlocker blocker(ui->equipmentTreeView->selectionModel());
        ui->equipmentTreeView->setCurrentIndex(initialIndex);
    }

    if (const Equipment* equipment = equipmentModel->equipmentForIndex(initialIndex)) {
        m_selectedEquipmentName = equipment->name();
    }

    displayEquipment(equipmentModel->equipmentForIndex(initialIndex), false);
    statusBar()->showMessage(tr("Monitoring dashboard ready"), 3000);
}

void MainWindow::configureParameterPanel() {
    ui->parametersTable->horizontalHeader()->setStretchLastSection(true);
    ui->parametersTable->verticalHeader()->hide();
    ui->parametersTable->setColumnWidth(0, 130);
    ui->breakerControlLabel->setVisible(false);
    ui->breakerStateCombo->setVisible(false);
    ui->breakerStateCombo->setCurrentText(tr("Closed"));
    ui->period->setFixedHeight(28);
    ui->day->setFixedHeight(28);
    ui->periodComboBox->setFixedHeight(32);
    ui->breakerStateCombo->setFixedHeight(32);
    ui->dateEdit->setFixedHeight(32);
    ui->groupBox->setMinimumHeight(90);
    ui->horizontalLayout_3->setAlignment(ui->period, Qt::AlignVCenter);
    ui->horizontalLayout_3->setAlignment(ui->periodComboBox, Qt::AlignVCenter);
    ui->horizontalLayout_3->setAlignment(ui->day, Qt::AlignVCenter);
    ui->horizontalLayout_3->setAlignment(ui->dateEdit, Qt::AlignVCenter);
    ui->temperatureControlLabel->setVisible(false);
    ui->temperatureSpinBox->setVisible(false);
    ui->applyTemperatureButton->setVisible(false);
    ui->temperatureSpinBox->setSuffix(tr(" C"));
    ui->periodComboBox->setItemData(0, 60);
    ui->periodComboBox->setItemData(1, 3600);
    ui->periodComboBox->setItemData(2, 24 * 3600);
    ui->temperatureSpinBox->setRange(-40.0, 150.0);
    ui->temperatureSpinBox->setDecimals(1);
    ui->temperatureSpinBox->setSingleStep(0.5);
    ui->eventLogTable->setFixedHeight(245);

    auto* copyShortcut = new QShortcut(QKeySequence::Copy, ui->eventLogTable);
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
            QTableWidgetItem* item = ui->eventLogTable->item(row, column);
            cells.append(item ? item->text() : QString());
        }
        QGuiApplication::clipboard()->setText(cells.join(QStringLiteral("\t")));
    });
}

void MainWindow::connectInteractions() {
    connect(EventLogger::instance(), &EventLogger::eventLogged, this,
            [this](EventLevel level, const QString& source, const QString& message) {
                appendEvent(level, source, message);
            });
    const QVector<EventRecord> storedEvents = EventLogger::instance()->records(QDateTime(), QDateTime(), false);
    const int firstVisibleEvent = qMax(0, storedEvents.size() - 1000);
    for (int index = firstVisibleEvent; index < storedEvents.size(); ++index) {
        const EventRecord& record = storedEvents.at(index);
        appendEvent(record.level, record.source, record.message, record.timestamp);
    }

    connect(ui->equipmentTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) {
                rememberCurrentSelection(current);
                displayEquipment(equipmentFromTreeIndex(current), true);
            });

    connect(diagramView, &SubstationDiagramView::equipmentActivated, this, [this](const QString& equipmentKey) {
        Equipment* selectedEquipment = findEquipmentByName(equipmentKey);
        displayEquipment(selectedEquipment, true);
        QModelIndex equipmentIndex = equipmentModel->indexForEquipment(selectedEquipment);
        if (equipmentIndex.isValid()) {
            QSignalBlocker blocker(ui->equipmentTreeView->selectionModel());
            ui->equipmentTreeView->setCurrentIndex(equipmentIndex);
        }
    });
    connect(ui->actionAbout, &QAction::triggered, this, [this]() {
        AboutDialog aboutDialog(this);
        aboutDialog.exec();
    });
    connect(ui->actionLoad, &QAction::triggered, this, [this]() {
        const QString layoutPath = QFileDialog::getOpenFileName(this, tr("Load scheme"), QDir::currentPath(),
                                                                tr("Substation schemes (*.json);;All files (*.*)"));
        if (!layoutPath.isEmpty()) {
            loadSubstationLayoutFile(layoutPath);
        }
    });
    connect(ui->actionSave, &QAction::triggered, this, [this]() { saveSubstationLayout(false); });
    connect(ui->actionSave_as, &QAction::triggered, this, [this]() { saveSubstationLayout(true); });
    connect(ui->actionSettings, &QAction::triggered, this, [this]() {
        SettingsDialog settingsDialog(this);
        settingsDialog.exec();
    });
    connect(ui->actionReport, &QAction::triggered, this, [this]() {
        ReportDialog reportDialog(this);
        reportDialog.exec();
    });

    connect(ui->periodComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { refreshMonitoringCharts(); });

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

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, [this](const QDate&) {
        if (ui->day->isChecked()) {
            refreshMonitoringCharts();
        }
    });

    connect(ui->breakerStateCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        const bool shouldBeClosed = text != QStringLiteral("Open");
        if (m_telemetryService->breakerClosed() == shouldBeClosed) {
            return;
        }

        logInfo(QStringLiteral("User Action"),
                tr("Breaker CB-1 set to %1").arg(shouldBeClosed ? tr("Closed") : tr("Open")));
        m_telemetryService->setBreakerClosed(shouldBeClosed);
    });
    connect(ui->applyTemperatureButton, &QPushButton::clicked, this, [this]() {
        const double temperature = ui->temperatureSpinBox->value();
        logInfo(QStringLiteral("User Action"), tr("Transformer temperature set to %1").arg(formatC(temperature)));
        m_telemetryService->setManualTemperature(temperature);
    });
}

void MainWindow::loadSubstationLayout() {
    const QStringList candidatePaths = {SubstationLayout::defaultLayoutPath(),
                                        QDir::current().filePath(QStringLiteral("substation_layout.json")),
                                        QDir::current().filePath(QStringLiteral("data/substation_layout.json"))};

    for (const QString& layoutPath : candidatePaths) {
        if (loadSubstationLayoutFile(layoutPath)) {
            return;
        }
    }
}

bool MainWindow::loadSubstationLayoutFile(const QString& layoutPath) {
    SubstationLayout::Layout layout;
    QString errorMessage;
    if (!SubstationLayout::loadFromFile(layoutPath, &layout, &errorMessage)) {
        statusBar()->showMessage(errorMessage, 5000);
        return false;
    }

    *m_baseLayout = layout;
    *m_liveLayout = layout;
    m_currentLayoutPath = layoutPath;
    updateLayoutFileLabel();

    if (m_hasLastSnapshot) {
        TelemetryLayoutMapper::apply(m_liveLayout, m_lastSnapshot);
        PowerFlowCalculator::annotateLayout(m_liveLayout, m_lastSnapshot.temperatureBySensor);
    }

    diagramView->setLayout(*m_liveLayout);
    equipmentModel->updateLayout(*m_liveLayout);
    m_selectedEquipmentName.clear();
    setupModelAndViews();
    statusBar()->showMessage(tr("Scheme loaded: %1").arg(QFileInfo(layoutPath).fileName()), 4000);
    return true;
}

bool MainWindow::saveSubstationLayout(bool saveAs) {
    QString layoutPath = m_currentLayoutPath;
    if (saveAs || layoutPath.isEmpty()) {
        layoutPath = QFileDialog::getSaveFileName(this, tr("Save scheme"),
                                                  layoutPath.isEmpty() ? QDir::currentPath() : layoutPath,
                                                  tr("Substation schemes (*.json);;All files (*.*)"));
        if (layoutPath.isEmpty()) {
            return false;
        }
        if (!layoutPath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
            layoutPath += QStringLiteral(".json");
        }
    }

    QString errorMessage;
    const SubstationLayout::Layout layout = diagramView->layoutWithCurrentPositions();
    if (!SubstationLayout::saveToFile(layoutPath, layout, &errorMessage)) {
        statusBar()->showMessage(errorMessage, 5000);
        return false;
    }

    *m_baseLayout = layout;
    *m_liveLayout = layout;
    m_currentLayoutPath = layoutPath;
    updateLayoutFileLabel();
    statusBar()->showMessage(tr("Scheme saved: %1").arg(QFileInfo(layoutPath).fileName()), 4000);
    return true;
}

void MainWindow::updateLayoutFileLabel() {
    if (!m_layoutFileLabel) {
        return;
    }

    const QString fileName =
        m_currentLayoutPath.isEmpty() ? tr("No scheme file") : QFileInfo(m_currentLayoutPath).fileName();
    m_layoutFileLabel->setText(tr("Scheme: %1").arg(fileName));
    m_layoutFileLabel->setToolTip(m_currentLayoutPath);
}

void MainWindow::setupMonitoringCharts() {
    ui->voltage->setScene(m_voltageScene);
    ui->current->setScene(m_currentScene);
    ui->temperature->setScene(m_temperatureScene);
    ui->voltage->setRenderHint(QPainter::Antialiasing, true);
    ui->current->setRenderHint(QPainter::Antialiasing, true);
    ui->temperature->setRenderHint(QPainter::Antialiasing, true);
    ui->voltage->setAlignment(Qt::AlignCenter);
    ui->current->setAlignment(Qt::AlignCenter);
    ui->temperature->setAlignment(Qt::AlignCenter);
    ui->voltage->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->voltage->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->current->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->current->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->temperature->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->temperature->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->current->setMinimumHeight(220);
    updateMonitoringChartLayout();
}

void MainWindow::updateMonitoringChartLayout() {
    if (!ui->chartsGrid) {
        return;
    }

    const bool compact = ui->monitoring->viewport()->width() < 760;
    ui->chartsGrid->removeWidget(ui->label);
    ui->chartsGrid->removeWidget(ui->voltage);
    ui->chartsGrid->removeWidget(ui->label_2);
    ui->chartsGrid->removeWidget(ui->current);

    if (compact) {
        ui->chartsGrid->addWidget(ui->label, 0, 0, Qt::AlignHCenter);
        ui->chartsGrid->addWidget(ui->voltage, 1, 0);
        ui->chartsGrid->addWidget(ui->label_2, 2, 0, Qt::AlignHCenter);
        ui->chartsGrid->addWidget(ui->current, 3, 0);
        ui->chartsGrid->setColumnStretch(0, 1);
    } else {
        ui->chartsGrid->addWidget(ui->label, 0, 0, Qt::AlignHCenter);
        ui->chartsGrid->addWidget(ui->label_2, 0, 1, Qt::AlignHCenter);
        ui->chartsGrid->addWidget(ui->voltage, 1, 0);
        ui->chartsGrid->addWidget(ui->current, 1, 1);
        ui->chartsGrid->setColumnStretch(0, 1);
        ui->chartsGrid->setColumnStretch(1, 1);
    }
}

void MainWindow::startTelemetryMonitoring() {
    connect(m_telemetryService, &TelemetryService::snapshotReady, this, &MainWindow::processSnapshot);
    connect(m_telemetryService, &TelemetryService::connectionStateChanged, this, &MainWindow::updateConnectionState);
    updateConnectionState(m_telemetryService->connectionState());
    m_telemetryService->start(1000);
}

void MainWindow::updateConnectionState(TelemetryService::ConnectionState state) {
    QString text;
    QString backgroundColor;
    QString foregroundColor;
    switch (state) {
    case TelemetryService::ConnectionState::Connected:
        text = tr("Connection: Connected");
        backgroundColor = QStringLiteral("#1f6f43");
        foregroundColor = QStringLiteral("#ffffff");
        break;
    case TelemetryService::ConnectionState::Connecting:
        text = tr("Connection: Connecting...");
        backgroundColor = QStringLiteral("#8a651b");
        foregroundColor = QStringLiteral("#ffffff");
        break;
    case TelemetryService::ConnectionState::Disconnected:
        text = tr("Connection: Disconnected");
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

int MainWindow::selectedWindowSeconds() const {
    if (ui->day && ui->day->isChecked()) {
        return 24 * 3600;
    }

    if (!ui->periodComboBox) {
        return 60;
    }

    return ui->periodComboBox->currentData().toInt();
}

QDateTime MainWindow::selectedEndTime() const {
    if (ui->day && ui->day->isChecked()) {
        return QDateTime(ui->dateEdit->date(), QTime(23, 59, 59));
    }

    return QDateTime::currentDateTime();
}

void MainWindow::emitTemperatureAlerts(const SensorSnapshot& snapshot) {
    constexpr double warningThreshold = 75.0;
    constexpr double criticalThreshold = 85.0;
    const double current = snapshot.transformerTemperatureC;
    if (current >= criticalThreshold) {
        logCritical(QStringLiteral("Telemetry"), tr("Transformer temperature critical: %1").arg(formatC(current)));
    } else if (current >= warningThreshold) {
        logWarning(QStringLiteral("Telemetry"), tr("Transformer temperature warning: %1").arg(formatC(current)));
    }
}

void MainWindow::processSnapshot(const SensorSnapshot& adjustedSnapshot) {
    if (!m_baseLayout || m_baseLayout->nodes.isEmpty()) {
        return;
    }

    const QDateTime sampleTime = QDateTime::currentDateTime();
    const double monitoredVoltageKv = adjustedSnapshot.breakerClosed ? adjustedSnapshot.sourceVoltageKv : 0.0;
    const double monitoredCurrentA = adjustedSnapshot.breakerClosed ? adjustedSnapshot.sourceCurrentA : 0.0;
    TelemetryHistory::appendSample(TelemetryHistory::SeriesKind::Voltage,
                                   TelemetrySample{sampleTime, monitoredVoltageKv});
    TelemetryHistory::appendSample(TelemetryHistory::SeriesKind::Current,
                                   TelemetrySample{sampleTime, monitoredCurrentA});
    TelemetryHistory::appendSample(TelemetryHistory::SeriesKind::Temperature,
                                   TelemetrySample{sampleTime, adjustedSnapshot.transformerTemperatureC});

    if (m_hasLastSnapshot) {
        if (m_lastSnapshot.breakerClosed != adjustedSnapshot.breakerClosed) {
            logWarning(QStringLiteral("Telemetry"),
                       adjustedSnapshot.breakerClosed ? tr("CB-1 closed") : tr("CB-1 opened"));
        }

        if (adjustedSnapshot.sourceVoltageKv > m_lastSnapshot.sourceVoltageKv + 2.0 ||
            adjustedSnapshot.sourceVoltageKv < m_lastSnapshot.sourceVoltageKv - 2.0) {
            logInfo(QStringLiteral("Telemetry"),
                    tr("Source voltage changed to %1").arg(formatKv(adjustedSnapshot.sourceVoltageKv)));
        }

        if (adjustedSnapshot.transformerTemperatureC > m_lastSnapshot.transformerTemperatureC + 1.5 ||
            adjustedSnapshot.transformerTemperatureC < m_lastSnapshot.transformerTemperatureC - 1.5) {
            logInfo(QStringLiteral("Telemetry"),
                    tr("Transformer temperature changed to %1").arg(formatC(adjustedSnapshot.transformerTemperatureC)));
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
        for (const QString& note : adjustedSnapshot.notes) {
            logInfo(QStringLiteral("Telemetry"), note);
        }
    }

    refreshMonitoringCharts();

    statusBar()->showMessage(tr("Live: %1 | %2 | %3")
                                 .arg(formatKv(adjustedSnapshot.sourceVoltageKv),
                                      formatA(adjustedSnapshot.sourceCurrentA),
                                      formatC(adjustedSnapshot.transformerTemperatureC)),
                             1500);
    m_lastSnapshot = adjustedSnapshot;
    m_hasLastSnapshot = true;
}

void MainWindow::refreshMonitoringCharts() {
    Equipment* selectedEquipment = findEquipmentByName(m_selectedEquipmentName);
    if (selectedEquipment) {
        updateMonitoringVisibility(selectedEquipment);
    }

    const int windowSeconds = selectedWindowSeconds();
    const QDateTime endTime = selectedEndTime();

    if (!ui->voltage->isHidden()) {
        MonitoringChart::render(m_voltageScene,
                                TelemetryHistory::series(TelemetryHistory::SeriesKind::Voltage, endTime, windowSeconds),
                                tr("Voltage"), QStringLiteral("kV"), QColor(0x56, 0xC2, 0xFF),
                                DiagramTheme::color(DiagramTheme::ColorRole::PanelText), windowSeconds);
    }

    if (!ui->current->isHidden()) {
        MonitoringChart::render(m_currentScene,
                                TelemetryHistory::series(TelemetryHistory::SeriesKind::Current, endTime, windowSeconds),
                                tr("Current"), QStringLiteral("A"), QColor(0xFF, 0xB8, 0x6B),
                                DiagramTheme::color(DiagramTheme::ColorRole::PanelText), windowSeconds);
    }

    if (!ui->temperature->isHidden()) {
        MonitoringChart::render(
            m_temperatureScene,
            TelemetryHistory::series(TelemetryHistory::SeriesKind::Temperature, endTime, windowSeconds),
            tr("Temperature"), QStringLiteral("C"), QColor(0xD9, 0x2D, 0x20),
            DiagramTheme::color(DiagramTheme::ColorRole::PanelText), windowSeconds);
    }
}

void MainWindow::updateMonitoringVisibility(Equipment* equipment) {
    const bool showVoltage = equipmentSupportsVoltage(equipment);
    const bool showCurrent = equipmentSupportsCurrent(equipment);
    const bool showTemperature = equipmentSupportsTemperatureControl(equipment);

    ui->label->setVisible(showVoltage);
    ui->voltage->setVisible(showVoltage);
    ui->label_2->setVisible(showCurrent);
    ui->current->setVisible(showCurrent);
    ui->label_3->setVisible(showTemperature);
    ui->temperature->setVisible(showTemperature);
    ui->groupBox->setVisible(showVoltage || showCurrent || showTemperature);
}

void MainWindow::rememberCurrentSelection(const QModelIndex& index) {
    if (Equipment* equipment = equipmentFromTreeIndex(index)) {
        m_selectedEquipmentName = equipment->name();
    }
}

void MainWindow::restoreSelection() {
    if (m_selectedEquipmentName.isEmpty()) {
        return;
    }

    Equipment* selectedEquipment = equipmentModel->equipmentByName(m_selectedEquipmentName);
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

void MainWindow::displayEquipment(Equipment* equipment, bool fromUserAction) {
    if (!equipment) {
        return;
    }

    m_selectedEquipmentName = equipment->name();

    ui->nameValueLabel->setText(equipment->name());
    ui->typeValueLabel->setText(translatedEquipmentType(equipment->type()));
    ui->statusValueLabel->setText(translatedEquipmentStatus(equipment->status()));
    ui->locationValueLabel->setText(equipment->location().isEmpty() ? "-" : equipment->location());
    ui->descriptionValueLabel->setText(equipment->description().isEmpty() ? "-" : equipment->description());
    updateBreakerControls(equipment);
    updateTemperatureControls(equipment);
    updateMonitoringVisibility(equipment);

    const auto parameters = equipment->parameters();
    QVector<QPair<QString, QString>> visibleParameters;
    visibleParameters.reserve(parameters.size());
    for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
        if (shouldDisplayParameter(equipment, it.key())) {
            visibleParameters.append(qMakePair(it.key(), it.value()));
        }
    }

    ui->parametersTable->clearContents();
    ui->parametersTable->setRowCount(visibleParameters.size());
    int row = 0;
    for (const auto& parameter : visibleParameters) {
        ui->parametersTable->setItem(row, 0, new QTableWidgetItem(translatedParameterName(parameter.first)));
        ui->parametersTable->setItem(row, 1, new QTableWidgetItem(parameter.second));
        ++row;
    }

    if (diagramView) {
        diagramView->selectEquipment(equipment->name());
    }

    if (fromUserAction) {
        logInfo(QStringLiteral("User Action"), tr("Selected equipment: %1").arg(equipment->name()));
    }
}

void MainWindow::updateBreakerControls(Equipment* equipment) {
    const bool isBreaker = equipment && equipment->name() == QStringLiteral("CB-1");
    ui->breakerControlLabel->setVisible(isBreaker);
    ui->breakerStateCombo->setVisible(isBreaker);

    if (!isBreaker) {
        return;
    }

    const QSignalBlocker blocker(ui->breakerStateCombo);
    ui->breakerStateCombo->setCurrentText(m_telemetryService->breakerClosed() ? tr("Closed") : tr("Open"));
}

void MainWindow::updateTemperatureControls(Equipment* equipment) {
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
    const double displayedTemperature = m_hasLastSnapshot ? m_telemetryService->displayedTemperature() : 68.0;
    ui->temperatureSpinBox->setValue(displayedTemperature);
}

void MainWindow::appendEvent(EventLevel level, const QString& source, const QString& message,
                             const QDateTime& timestamp) {
    if (!ui->eventLogTable) {
        return;
    }

    int row = ui->eventLogTable->rowCount();
    ui->eventLogTable->insertRow(row);

    QTableWidgetItem* levelItem = new QTableWidgetItem();
    switch (level) {
    case EventLevel::Info:
        levelItem->setText(tr("Info"));
        break;
    case EventLevel::Warning:
        levelItem->setText(tr("Warning"));
        levelItem->setForeground(Qt::yellow);
        break;
    case EventLevel::Critical:
        levelItem->setText(tr("Critical"));
        levelItem->setForeground(Qt::red);
        break;
    }
    const QDateTime eventTime = timestamp.isValid() ? timestamp : QDateTime::currentDateTime();
    QTableWidgetItem* timeItem = new QTableWidgetItem(eventTime.toString("yyyy-MM-dd HH:mm:ss"));
    ui->eventLogTable->setItem(row, 1, levelItem);
    ui->eventLogTable->setItem(row, 0, timeItem);
    QString displaySource = source;
    if (source == QStringLiteral("Telemetry")) {
        displaySource = tr("Telemetry");
    } else if (source == QStringLiteral("User Action")) {
        displaySource = tr("User Action");
    }
    ui->eventLogTable->setItem(row, 2, new QTableWidgetItem(displaySource));
    ui->eventLogTable->setItem(row, 3, new QTableWidgetItem(message));

    constexpr int maximumVisibleEvents = 1000;
    while (ui->eventLogTable->rowCount() > maximumVisibleEvents) {
        ui->eventLogTable->removeRow(0);
    }
    row = ui->eventLogTable->rowCount() - 1;
    ui->eventLogTable->selectRow(row);
    ui->eventLogTable->scrollToBottom();
}

Equipment* MainWindow::equipmentFromTreeIndex(const QModelIndex& index) const {
    return equipmentModel->equipmentForIndex(index);
}

Equipment* MainWindow::findEquipmentByName(const QString& name) const {
    return equipmentModel->equipmentByName(name);
}

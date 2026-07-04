#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "equipment.h"
#include "equipmenttreemodel.h"
#include "diagramtheme.h"
#include "substationdiagramview.h"
#include "substationlayout.h"
#include "aboutdialog.h"
#include "eventlogger.h"
#include "settingsdialog.h"

#include <QHeaderView>
#include <QDateTime>
#include <QPalette>
#include <QItemSelectionModel>
#include <QList>
#include <QStringList>
#include <QSplitter>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QAbstractItemView>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , equipmentModel(new EquipmentTreeModel(this))
    , diagramView(nullptr)
{
    ui->setupUi(this);
    diagramView = ui->graphicsView;

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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupModelAndViews()
{
    const QModelIndex initialIndex = equipmentModel->index(0, 0);
    if (initialIndex.isValid()) {
        QSignalBlocker blocker(ui->equipmentTreeView->selectionModel());
        ui->equipmentTreeView->setCurrentIndex(initialIndex);
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

    equipmentModel->setLayout(layout);
    diagramView->setLayout(layout);
}

void MainWindow::displayEquipment(Equipment *equipment, bool fromUserAction)
{
    if (!equipment) {
        return;
    }

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

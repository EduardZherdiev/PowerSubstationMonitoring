#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QApplication>
#include <QPushButton>

namespace {

AppSettings::ThemeMode themeFromRadioButtons(const Ui::SettingsDialog *ui)
{
    if (ui->darkTheme->isChecked()) {
        return AppSettings::ThemeMode::Dark;
    }

    if (ui->lightTheme->isChecked()) {
        return AppSettings::ThemeMode::Light;
    }

    return AppSettings::ThemeMode::System;
}

void selectThemeRadio(Ui::SettingsDialog *ui, AppSettings::ThemeMode theme)
{
    ui->systemTheme->setChecked(theme == AppSettings::ThemeMode::System);
    ui->lightTheme->setChecked(theme == AppSettings::ThemeMode::Light);
    ui->darkTheme->setChecked(theme == AppSettings::ThemeMode::Dark);
}

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_settings(AppSettings::load())
{
    ui->setupUi(this);
    ui->pushButton_2->setEnabled(true);

    connect(ui->pushButton_2, &QPushButton::clicked, this, &SettingsDialog::accept);
    connect(ui->pushButton, &QPushButton::clicked, this, &SettingsDialog::reject);

    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::loadSettings()
{
    selectThemeRadio(ui, m_settings.theme);
}

AppSettings::ThemeMode SettingsDialog::selectedTheme() const
{
    return themeFromRadioButtons(ui);
}

void SettingsDialog::accept()
{
    m_settings.theme = selectedTheme();
    AppSettings::save(m_settings);
    AppSettings::applyTheme(*qApp, m_settings.theme);
    QDialog::accept();
}

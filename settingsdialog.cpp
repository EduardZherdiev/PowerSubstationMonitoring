#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QApplication>
#include <QEvent>
#include <QPushButton>
#include <QTranslator>

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
    ui->languageComboBox->setItemData(0, QStringLiteral("en"));
    ui->languageComboBox->setItemData(1, QStringLiteral("uk"));
    ui->systemTheme->setFixedHeight(28);
    ui->lightTheme->setFixedHeight(28);
    ui->darkTheme->setFixedHeight(28);
    ui->languageComboBox->setFixedHeight(32);
    ui->languageLayout->setAlignment(ui->languageComboBox, Qt::AlignVCenter);
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
    const int languageIndex = ui->languageComboBox->findData(m_settings.language);
    ui->languageComboBox->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
}

AppSettings::ThemeMode SettingsDialog::selectedTheme() const
{
    return themeFromRadioButtons(ui);
}

QString SettingsDialog::selectedLanguage() const
{
    const QString language = ui->languageComboBox->currentData().toString();
    return language.isEmpty() ? QStringLiteral("en") : language;
}

void SettingsDialog::accept()
{
    m_settings.theme = selectedTheme();
    m_settings.language = selectedLanguage();
    AppSettings::save(m_settings);
    AppSettings::applyTheme(*qApp, m_settings.theme);

    if (auto *translator = qApp->findChild<QTranslator *>(
            QStringLiteral("applicationTranslator"))) {
        AppSettings::applyLanguage(*qApp, m_settings.language, *translator);
    }

    QDialog::accept();
}

void SettingsDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        loadSettings();
    }
    QDialog::changeEvent(event);
}

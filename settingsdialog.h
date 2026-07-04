#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "appsettings.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

public slots:
    void accept() override;

private:
    void loadSettings();
    AppSettings::ThemeMode selectedTheme() const;

    Ui::SettingsDialog *ui;
    AppSettings::Settings m_settings;
};

#endif // SETTINGSDIALOG_H

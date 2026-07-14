#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QEvent>

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

protected:
    void changeEvent(QEvent *event) override;

private:
    void loadSettings();
    AppSettings::ThemeMode selectedTheme() const;
    QString selectedLanguage() const;

    Ui::SettingsDialog *ui;
    AppSettings::Settings m_settings;
};

#endif // SETTINGSDIALOG_H

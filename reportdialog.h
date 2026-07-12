#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include "eventlogger.h"

#include <QDialog>

namespace Ui {
class ReportDialog;
}

class ReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReportDialog(QWidget *parent = nullptr);
    ~ReportDialog();

private:
    QVector<EventRecord> selectedRecords() const;
    QString reportText(const QVector<EventRecord> &records) const;
    QString temperaturePeriodsText(const QVector<EventRecord> &records) const;
    void refreshPreview();
    void exportTxt();
    void exportPdf();

    Ui::ReportDialog *ui;
};

#endif // REPORTDIALOG_H

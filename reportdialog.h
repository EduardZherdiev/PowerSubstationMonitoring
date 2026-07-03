#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include <QScrollArea>

namespace Ui {
class ReportDialog;
}

class ReportDialog : public QScrollArea
{
    Q_OBJECT

public:
    explicit ReportDialog(QWidget *parent = nullptr);
    ~ReportDialog();

private:
    Ui::ReportDialog *ui;
};

#endif // REPORTDIALOG_H

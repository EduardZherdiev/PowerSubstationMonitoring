#include "reportdialog.h"
#include "ui_reportdialog.h"

ReportDialog::ReportDialog(QWidget *parent)
    : QScrollArea(parent)
    , ui(new Ui::ReportDialog)
{
    ui->setupUi(this);
}

ReportDialog::~ReportDialog()
{
    delete ui;
}

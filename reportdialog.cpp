#include "reportdialog.h"
#include "ui_reportdialog.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrinter>
#include <QTextDocument>
#include <QTextStream>

namespace {

QString levelText(EventLevel level)
{
    switch (level) {
    case EventLevel::Info:
        return QStringLiteral("Info");
    case EventLevel::Warning:
        return QStringLiteral("Warning");
    case EventLevel::Critical:
        return QStringLiteral("Critical");
    }
    return QStringLiteral("Info");
}

} // namespace

ReportDialog::ReportDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReportDialog)
{
    ui->setupUi(this);

    ui->fromEdit->setDateTime(QDateTime::currentDateTime().addDays(-1));
    ui->toEdit->setDateTime(QDateTime::currentDateTime());

    connect(ui->fromEdit, &QDateTimeEdit::dateTimeChanged, this, [this]() {
        refreshPreview();
    });
    connect(ui->toEdit, &QDateTimeEdit::dateTimeChanged, this, [this]() {
        refreshPreview();
    });
    connect(ui->exportTxtButton, &QPushButton::clicked, this, &ReportDialog::exportTxt);
    connect(ui->exportPdfButton, &QPushButton::clicked, this, &ReportDialog::exportPdf);

    refreshPreview();
}

ReportDialog::~ReportDialog()
{
    delete ui;
}

QVector<EventRecord> ReportDialog::selectedRecords() const
{
    return EventLogger::instance()->records(ui->fromEdit->dateTime(), ui->toEdit->dateTime(), true);
}

QString ReportDialog::reportText(const QVector<EventRecord> &records) const
{
    QString text;
    QTextStream stream(&text);
    stream << "Power Substation Monitoring Report\n";
    stream << "Period: " << ui->fromEdit->dateTime().toString("yyyy-MM-dd HH:mm:ss")
           << " - " << ui->toEdit->dateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    stream << "Included levels: Warning, Critical\n";
    stream << "Total events: " << records.size() << "\n\n";

    if (records.isEmpty()) {
        stream << "No warning or critical events for the selected period.\n";
        return text;
    }

    for (const EventRecord &record : records) {
        stream << record.timestamp.toString("yyyy-MM-dd HH:mm:ss")
               << " | " << levelText(record.level)
               << " | " << record.source
               << " | " << record.message << "\n";
    }

    return text;
}

void ReportDialog::refreshPreview()
{
    ui->previewEdit->setPlainText(reportText(selectedRecords()));
}

void ReportDialog::exportTxt()
{
    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          QStringLiteral("Export TXT Report"),
                                                          QStringLiteral("event-report.txt"),
                                                          QStringLiteral("Text files (*.txt)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Export failed"), QStringLiteral("Could not write TXT report."));
        return;
    }

    QTextStream stream(&file);
    stream << reportText(selectedRecords());
}

void ReportDialog::exportPdf()
{
    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          QStringLiteral("Export PDF Report"),
                                                          QStringLiteral("event-report.pdf"),
                                                          QStringLiteral("PDF files (*.pdf)"));
    if (filePath.isEmpty()) {
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageMargins(QMarginsF(12.0, 12.0, 12.0, 12.0));

    QTextDocument document;
    document.setPlainText(reportText(selectedRecords()));
    document.print(&printer);
}

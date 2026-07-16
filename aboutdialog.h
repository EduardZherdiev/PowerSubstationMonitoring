#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QEvent>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog {
    Q_OBJECT

  public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog();

  protected:
    void changeEvent(QEvent* event) override;

  private:
    Ui::AboutDialog* ui;
};

#endif // ABOUTDIALOG_H

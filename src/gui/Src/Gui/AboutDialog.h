#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

class UpdateChecker;

namespace Ui
{
    class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = 0);
    ~AboutDialog();

private slots:
    void on_btnCheckUpdates_clicked();

private:
    Ui::AboutDialog* ui;
    UpdateChecker* mUpdateChecker;
};

#endif // ABOUTDIALOG_H

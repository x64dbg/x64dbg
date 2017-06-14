#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

class UpdateChecker;

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = 0);
    ~AboutDialog();

private slots:
    void on_lblWebsite_linkActivated(const QString &link);
    void on_lblVersion_7_linkActivated(const QString &link);
    void on_btnCheckUpdates_clicked();
    void on_lblAbout_2_linkActivated(const QString &link);

private:
    Ui::AboutDialog *ui;
    UpdateChecker* mUpdateChecker;
};

#endif // ABOUTDIALOG_H

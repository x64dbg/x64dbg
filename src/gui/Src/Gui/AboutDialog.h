#pragma once

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
    explicit AboutDialog(UpdateChecker* updateChecker, QWidget* parent = 0);
    ~AboutDialog();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void on_btnCheckUpdates_clicked();

private:
    Ui::AboutDialog* ui;
    UpdateChecker* mUpdateChecker;
};

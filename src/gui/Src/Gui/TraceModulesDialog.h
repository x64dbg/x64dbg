#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>

namespace Ui
{
class TraceModulesDialog;
}

class TraceModulesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TraceModulesDialog(QWidget* parent = nullptr);
    ~TraceModulesDialog();

    void loadModules();

private slots:
    void on_buttonOk_clicked();
    void on_buttonCancel_clicked();
    void on_buttonClearIncluded_clicked();
    void on_buttonClearExcluded_clicked();

private:
    void saveModules();
    Ui::TraceModulesDialog* ui;
};
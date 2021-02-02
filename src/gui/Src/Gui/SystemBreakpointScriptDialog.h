#pragma once

#include <QDialog>

namespace Ui
{
    class SystemBreakpointScriptDialog;
}

class SystemBreakpointScriptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SystemBreakpointScriptDialog(QWidget* parent = nullptr);
    ~SystemBreakpointScriptDialog();

private slots:
    void on_pushButtonGlobal_clicked();
    void on_pushButtonDebuggee_clicked();
    void on_openGlobal_clicked();
    void on_openDebuggee_clicked();
    void on_SystemBreakpointScriptDialog_accepted();

private:
    Ui::SystemBreakpointScriptDialog* ui;
};


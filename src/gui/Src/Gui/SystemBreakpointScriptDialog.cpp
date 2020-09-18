#include "SystemBreakpointScriptDialog.h"
#include "ui_SystemBreakpointScriptDialog.h"
#include "Bridge.h"
#include "Configuration.h"
#include <QDirModel>
#include <QFileDialog>
#include <QCompleter>

SystemBreakpointScriptDialog::SystemBreakpointScriptDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SystemBreakpointScriptDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    auto dirCompleter = [](QLineEdit * lineEdit)
    {
        QCompleter* completer = new QCompleter(lineEdit);
        completer->setModel(new QDirModel(completer));
        lineEdit->setCompleter(completer);
    };
    dirCompleter(ui->lineEditGlobal);
    dirCompleter(ui->lineEditDebuggee);

    {
        char globalChar[MAX_SETTING_SIZE];
        if(BridgeSettingGet("Engine", "InitializeScript", globalChar))
            ui->lineEditGlobal->setText(globalChar);
    }

    if(DbgIsDebugging())
    {
        char moduleName[MAX_MODULE_SIZE];
        if(DbgFunctions()->ModNameFromAddr(DbgValFromString("mod.main()"), moduleName, true))
        {
            ui->groupBoxDebuggee->setTitle(tr("2. System breakpoint script for %1").arg(moduleName));
        }

        ui->lineEditDebuggee->setText(DbgFunctions()->DbgGetDebuggeeInitScript());
    }
    else
    {
        ui->groupBoxDebuggee->setEnabled(false);
    }

    Config()->setupWindowPos(this);
}

SystemBreakpointScriptDialog::~SystemBreakpointScriptDialog()
{
    delete ui;
}

void SystemBreakpointScriptDialog::on_pushButtonGlobal_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, ui->groupBoxGlobal->title(), ui->lineEditGlobal->text(), tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(!file.isEmpty())
        ui->lineEditGlobal->setText(QDir::toNativeSeparators(file));
}

void SystemBreakpointScriptDialog::on_pushButtonDebuggee_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, ui->groupBoxDebuggee->title(), ui->lineEditDebuggee->text(), tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(!file.isEmpty())
        ui->lineEditDebuggee->setText(QDir::toNativeSeparators(file));
}

void SystemBreakpointScriptDialog::on_SystemBreakpointScriptDialog_accepted()
{
    BridgeSettingSet("Engine", "InitializeScript", ui->lineEditGlobal->text().toUtf8().constData());
    if(ui->groupBoxDebuggee->isEnabled())
        DbgFunctions()->DbgSetDebuggeeInitScript(ui->lineEditDebuggee->text().toUtf8().constData());
}

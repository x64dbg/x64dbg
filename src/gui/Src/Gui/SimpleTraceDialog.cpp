#include "SimpleTraceDialog.h"
#include "ui_SimpleTraceDialog.h"
#include "Bridge.h"
#include <QMessageBox>
#include "BrowseDialog.h"

SimpleTraceDialog::SimpleTraceDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SimpleTraceDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    duint setting;
    if(!BridgeSettingGetUint("Engine", "MaxTraceCount", &setting))
        setting = 50000;
    ui->spinMaxTraceCount->setValue(int(setting));
    ui->editBreakCondition->setPlaceholderText(tr("Example: %1").arg("eax == 0 && ebx == 0"));
    ui->editLogText->setPlaceholderText(tr("Example: %1").arg("0x{p:cip} {i:cip}"));
    ui->editLogCondition->setPlaceholderText(tr("Example: %1").arg("eax == 0 && ebx == 0"));
    ui->editCommandText->setPlaceholderText(tr("Example: %1").arg("eax=4;StepOut"));
    ui->editCommandCondition->setPlaceholderText(tr("Example: %1").arg("eax == 0 && ebx == 0"));
    ui->editSwitchCondition->setPlaceholderText(tr("Example: %1").arg("mod.party(dis.branchdest(cip)) == 1"));
}

SimpleTraceDialog::~SimpleTraceDialog()
{
    delete ui;
}

void SimpleTraceDialog::setTraceCommand(const QString & command)
{
    mTraceCommand = command;
}

static QString escapeText(QString str)
{
    str.replace(QChar('\\'), QString("\\\\"));
    str.replace(QChar('"'), QString("\\\""));
    return str;
}

void SimpleTraceDialog::on_btnOk_clicked()
{
    if(!mLogFile.isEmpty() && ui->editLogText->text().isEmpty())
    {
        QMessageBox msgyn(QMessageBox::Warning, tr("Trace log file"),
                          tr("It appears you have set the log file, but not the log text. <b>This will result in an empty log</b>. Do you really want to continue?"), QMessageBox::Yes | QMessageBox::No, this);
        msgyn.setWindowIcon(DIcon("compile-warning.png"));
        msgyn.setParent(this, Qt::Dialog);
        msgyn.setWindowFlags(msgyn.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msgyn.exec() == QMessageBox::No)
            return;
    }
    auto logText = ui->editLogText->addHistoryClear();
    auto logCondition = ui->editLogCondition->addHistoryClear();
    if(!DbgCmdExecDirect(QString("TraceSetLog \"%1\", \"%2\"").arg(escapeText(logText), escapeText(logCondition)).toUtf8().constData()))
    {
        SimpleWarningBox(this, tr("Error"), tr("Failed to set log text/condition!"));
        return;
    }
    auto commandText = ui->editCommandText->addHistoryClear();
    auto commandCondition = ui->editCommandCondition->addHistoryClear();
    if(!DbgCmdExecDirect(QString("TraceSetCommand \"%1\", \"%2\"").arg(escapeText(commandText), escapeText(commandCondition)).toUtf8().constData()))
    {
        SimpleWarningBox(this, tr("Error"), tr("Failed to set command text/condition!"));
        return;
    }
    auto switchCondition = ui->editSwitchCondition->addHistoryClear();
    if(!DbgCmdExecDirect(QString("TraceSetSwitchCondition \"%1\"").arg(escapeText(switchCondition)).toUtf8().constData()))
    {
        SimpleWarningBox(this, tr("Error"), tr("Failed to set switch condition!"));
        return;
    }
    if(!DbgCmdExecDirect(QString("TraceSetLogFile \"%1\"").arg(escapeText(mLogFile)).toUtf8().constData()))
    {
        SimpleWarningBox(this, tr("Error"), tr("Failed to set log file!"));
        return;
    }
    auto breakCondition = ui->editBreakCondition->addHistoryClear();
    auto maxTraceCount = ui->spinMaxTraceCount->value();
    if(!DbgCmdExecDirect(QString("%1 \"%2\", .%3").arg(mTraceCommand, escapeText(breakCondition)).arg(maxTraceCount).toUtf8().constData()))
    {
        SimpleWarningBox(this, tr("Error"), tr("Failed to start trace!"));
        return;
    }
    accept();
}

void SimpleTraceDialog::on_btnLogFile_clicked()
{
    BrowseDialog browse(this, tr("Trace log file"), tr("Enter the path to the log file."), tr("Log Files (*.txt *.log);;All Files (*.*)"), QCoreApplication::applicationDirPath(), true);
    if(browse.exec() == QDialog::Accepted)
        mLogFile = browse.path;
    else
        mLogFile.clear();
}

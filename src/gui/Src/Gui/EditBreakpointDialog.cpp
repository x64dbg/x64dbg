#include "EditBreakpointDialog.h"
#include "ui_EditBreakpointDialog.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include "Configuration.h"

EditBreakpointDialog::EditBreakpointDialog(QWidget* parent, const BRIDGEBP & bp)
    : QDialog(parent),
      ui(new Ui::EditBreakpointDialog),
      mBp(bp)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    switch(bp.type)
    {
    case bp_dll:
        setWindowTitle(tr("Edit DLL Breakpoint %1").arg(QString(bp.mod)));
        break;
    case bp_normal:
        setWindowTitle(tr("Edit Breakpoint %1").arg(getSymbolicName(bp.addr)));
        break;
    case bp_hardware:
        setWindowTitle(tr("Edit Hardware Breakpoint %1").arg(getSymbolicName(bp.addr)));
        break;
    case bp_memory:
        setWindowTitle(tr("Edit Memory Breakpoint %1").arg(getSymbolicName(bp.addr)));
        break;
    case bp_exception:
        setWindowTitle(tr("Edit Exception Breakpoint %1").arg(getSymbolicName(bp.addr)));
        break;
    default:
        setWindowTitle(tr("Edit Breakpoint %1").arg(getSymbolicName(bp.addr)));
        break;
    }
    setWindowIcon(DIcon("breakpoint"));
    loadFromBp();

    Config()->setupWindowPos(this);
}

EditBreakpointDialog::~EditBreakpointDialog()
{
    Config()->saveWindowPos(this);
    delete ui;
}

void EditBreakpointDialog::loadFromBp()
{
    ui->editName->setText(mBp.name);
    ui->spinHitCount->setValue(mBp.hitCount);
    ui->editBreakCondition->setText(mBp.breakCondition);
    ui->checkBoxFastResume->setChecked(mBp.fastResume);
    ui->checkBoxSilent->setChecked(mBp.silent);
    ui->checkBoxSingleshoot->setChecked(mBp.singleshoot);
    ui->editLogText->setText(mBp.logText);
    ui->editLogCondition->setText(mBp.logCondition);
    ui->editCommandText->setText(mBp.commandText);
    ui->editCommandCondition->setText(mBp.commandCondition);
}

template<typename T>
void copyTruncate(T & dest, QString src)
{
    src.replace(QChar('\\'), QString("\\\\"));
    src.replace(QChar('"'), QString("\\\""));
    strncpy_s(dest, src.toUtf8().constData(), _TRUNCATE);
}

void EditBreakpointDialog::on_editName_textEdited(const QString & arg1)
{
    copyTruncate(mBp.name, arg1);
}

void EditBreakpointDialog::on_editBreakCondition_textEdited(const QString & arg1)
{
    copyTruncate(mBp.breakCondition, arg1);
}

void EditBreakpointDialog::on_editLogText_textEdited(const QString & arg1)
{
    ui->checkBoxSilent->setChecked(true);
    copyTruncate(mBp.logText, arg1);
}

void EditBreakpointDialog::on_editLogCondition_textEdited(const QString & arg1)
{
    copyTruncate(mBp.logCondition, arg1);
}

void EditBreakpointDialog::on_editCommandText_textEdited(const QString & arg1)
{
    copyTruncate(mBp.commandText, arg1);
}

void EditBreakpointDialog::on_editCommandCondition_textEdited(const QString & arg1)
{
    copyTruncate(mBp.commandCondition, arg1);
}

void EditBreakpointDialog::on_checkBoxFastResume_toggled(bool checked)
{
    mBp.fastResume = checked;
}

void EditBreakpointDialog::on_spinHitCount_valueChanged(int arg1)
{
    mBp.hitCount = arg1;
}

void EditBreakpointDialog::on_checkBoxSilent_toggled(bool checked)
{
    mBp.silent = checked;
}

void EditBreakpointDialog::on_checkBoxSingleshoot_toggled(bool checked)
{
    mBp.singleshoot = checked;
}

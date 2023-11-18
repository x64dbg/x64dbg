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
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedHeight(sizeHint().height()); // resizable only horizontally

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

    connect(this, SIGNAL(accepted()), this, SLOT(acceptedSlot()));

    Config()->loadWindowGeometry(this);
}

EditBreakpointDialog::~EditBreakpointDialog()
{
    Config()->saveWindowGeometry(this);
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
    src = DbgCmdEscape(std::move(src));
    strncpy_s(dest, src.toUtf8().constData(), _TRUNCATE);
}

void EditBreakpointDialog::on_editLogText_textEdited(const QString & arg1)
{
    Q_UNUSED(arg1);
    ui->checkBoxSilent->setChecked(true);
}

void EditBreakpointDialog::acceptedSlot()
{
    copyTruncate(mBp.breakCondition, ui->editBreakCondition->text());
    copyTruncate(mBp.logText, ui->editLogText->text());
    copyTruncate(mBp.logCondition, ui->editLogCondition->text());
    copyTruncate(mBp.commandText, ui->editCommandText->text());
    copyTruncate(mBp.commandCondition, ui->editCommandCondition->text());
    copyTruncate(mBp.name, ui->editName->text());

    mBp.singleshoot = ui->checkBoxSingleshoot->isChecked();
    mBp.fastResume = ui->checkBoxFastResume->isChecked();
    mBp.hitCount = ui->spinHitCount->value();
    mBp.silent = ui->checkBoxSilent->isChecked();
}

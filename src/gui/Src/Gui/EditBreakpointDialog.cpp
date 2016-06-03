#include "EditBreakpointDialog.h"
#include "ui_EditBreakpointDialog.h"
#include "StringUtil.h"

EditBreakpointDialog::EditBreakpointDialog(QWidget* parent, const BRIDGEBP & bp)
    : QDialog(parent),
      ui(new Ui::EditBreakpointDialog),
      mBp(bp)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
    setWindowTitle(QString("Edit Breakpoint %1").arg(ToHexString(bp.addr)));
    setWindowIcon(QIcon(":/icons/images/breakpoint.png"));
    loadFromBp();
}

EditBreakpointDialog::~EditBreakpointDialog()
{
    delete ui;
}

void EditBreakpointDialog::loadFromBp()
{
    ui->editName->setText(mBp.name);
    ui->spinHitCount->setValue(mBp.hitCount);
    ui->editBreakCondition->setText(mBp.breakCondition);
    ui->checkBoxFastResume->setChecked(mBp.fastResume);
    ui->editLogText->setText(mBp.logText);
    ui->editLogCondition->setText(mBp.logCondition);
    ui->editCommandText->setText(mBp.commandText);
    ui->editCommandCondition->setText(mBp.commandCondition);
}

template<typename T>
void copyTruncate(T dest, const QString & src)
{
    strcpy_s(dest, _TRUNCATE, src.toUtf8().constData());
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

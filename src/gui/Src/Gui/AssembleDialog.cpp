#include "AssembleDialog.h"
#include "ui_AssembleDialog.h"
#include <QMessageBox>

bool AssembleDialog::bWarningShowedOnce = false;

AssembleDialog::AssembleDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AssembleDialog)
{
    ui->setupUi(this);
    setModal(true);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
    bKeepSizeChecked = false;
    bFillWithNopsChecked = false;
    selectedInstrVa = 0;

}

AssembleDialog::~AssembleDialog()
{
    delete ui;
}

void AssembleDialog::setTextEditValue(const QString &text)
{
    ui->lineEdit->setText(text);
}

void AssembleDialog::setKeepSizeChecked(bool checked)
{
    ui->checkBoxKeepSize->setChecked(checked);
    bKeepSizeChecked = checked;
}

void AssembleDialog::setKeepSizeLabel(const QString &text)
{
    ui->labelKeepSize->setText(text);
}

void AssembleDialog::setFillWithNopsChecked(bool checked)
{
    ui->checkBoxFillWithNops->setChecked(checked);
    bFillWithNopsChecked = checked;
}

void AssembleDialog::setFillWithNopsLabel(const QString &text)
{
    ui->labelFillWithNops->setText(text);
}

void AssembleDialog::setSelectedInstrVa(const duint va)
{
    this->selectedInstrVa = va;
}

void AssembleDialog::compareTypedInstructionToSelected()
{
    char error[MAX_ERROR_SIZE] = "";
    BASIC_INSTRUCTION_INFO basicInstrInfo;
    int typedInstructionSize = 0, selectedInstructionSize = 0;

    // Get selected instruction info (size here)
    DbgDisasmFastAt(this->selectedInstrVa, &basicInstrInfo);
    selectedInstructionSize = basicInstrInfo.size;

    // Get typed in instruction size
    if(!DbgFunctions()->Assemble(0, NULL, &typedInstructionSize, editText.toUtf8().constData(), error)  || selectedInstructionSize == 0)
    {
        this->setKeepSizeLabel("<font color='orange'><b>Instruction decoding error : " + QString(error) + "</b></font>");
        return;
    }


    if(typedInstructionSize > selectedInstructionSize)
    {
        int sizeDifference = typedInstructionSize - selectedInstructionSize;
        QString errorMessage = "<font color='red'><b>Instruction bigger by " + QString::number(sizeDifference);
        (sizeDifference == 1) ? errorMessage += QString(" byte</b></font>") : errorMessage += QString(" bytes</b></font>");

        this->setKeepSizeLabel(errorMessage);
        this->setOkButtonEnabled(false);
    }
    else
    {
        int sizeDifference = selectedInstructionSize - typedInstructionSize;
        QString message;
        if(!sizeDifference)
            message = "<font color='#00cc00'><b>Instruction is same size</b></font>";
        else
        {
            message = "<font color='#00cc00'><b>Instruction smaller by " + QString::number(sizeDifference);
            (sizeDifference == 1) ? message += QString(" byte</b></font>") : message += QString(" bytes</b></font>");
        }
        this->setKeepSizeLabel(message);
        this->setOkButtonEnabled(true);
    }
}

void AssembleDialog::setOkButtonEnabled(bool enabled)
{
    ui->pushButtonOk->setEnabled(enabled);
}

void AssembleDialog::on_lineEdit_textChanged(const QString &arg1)
{
    editText = arg1;

    if(ui->checkBoxKeepSize->isChecked() && editText.size())
        compareTypedInstructionToSelected();
}

void AssembleDialog::on_checkBoxKeepSize_clicked(bool checked)
{
    // If first time ticking this checkbox, warn user about possible short dialog freeze when typing invalid instruction
    if(checked && !AssembleDialog::bWarningShowedOnce)
    {
        int answer = QMessageBox::question(this, "Possible dialog freeze", "Enabling this option may cause this dialog to freeze for a short amount of time when typing invalid instruction. Do you still want to enable this ?", QMessageBox::Yes | QMessageBox::No);
        if(answer == QMessageBox::No)
        {
            ui->checkBoxKeepSize->setChecked(false);
            return;
        }
        else
            AssembleDialog::bWarningShowedOnce = true;
    }

    if(checked && editText.size())
        compareTypedInstructionToSelected();
    else
    {
        ui->labelKeepSize->setText("");
        ui->pushButtonOk->setEnabled(true);
    }
    bKeepSizeChecked = checked;
}

void AssembleDialog::on_checkBoxFillWithNops_clicked(bool checked)
{
    bFillWithNopsChecked = checked;
}

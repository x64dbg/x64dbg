#include "AssembleDialog.h"
#include "ui_AssembleDialog.h"
#include <QMessageBox>

bool AssembleDialog::bWarningShowedOnce = false;

AssembleDialog::AssembleDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AssembleDialog)
{
    ui->setupUi(this);
    setModal(true);
    setFixedSize(this->size()); //fixed size

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif

    mSelectedInstrVa = 0;
    bKeepSizeChecked = false;
    bFillWithNopsChecked = false;

    mValidateThread = new ValidateExpressionThread(this);
    mValidateThread->setOnExpressionChangedCallback(std::bind(&AssembleDialog::validateInstruction, this, std::placeholders::_1));

    connect(ui->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textChangedSlot(QString)));
    connect(mValidateThread, SIGNAL(instructionChanged(dsint, QString)), this, SLOT(instructionChangedSlot(dsint, QString)));
}

AssembleDialog::~AssembleDialog()
{
    delete ui;
}

void AssembleDialog::setTextEditValue(const QString & text)
{
    ui->lineEdit->setText(text);
    ui->lineEdit->selectAll();
}

void AssembleDialog::setKeepSizeChecked(bool checked)
{
    ui->checkBoxKeepSize->setChecked(checked);
    bKeepSizeChecked = checked;
}

void AssembleDialog::setKeepSizeLabel(const QString & text)
{
    ui->labelKeepSize->setText(text);
}

void AssembleDialog::setFillWithNopsChecked(bool checked)
{
    ui->checkBoxFillWithNops->setChecked(checked);
    bFillWithNopsChecked = checked;
}

void AssembleDialog::setFillWithNopsLabel(const QString & text)
{
    ui->labelFillWithNops->setText(text);
}

void AssembleDialog::setSelectedInstrVa(const duint va)
{
    this->mSelectedInstrVa = va;
}


void AssembleDialog::setOkButtonEnabled(bool enabled)
{
    ui->pushButtonOk->setEnabled(enabled);
}

void AssembleDialog::validateInstruction(QString expression)
{
    //void instructionChanged(bool validInstruction, dsint sizeDifference, QString error)
    dsint sizeDifference = 0;
    int typedInstructionSize = 0;
    int selectedInstructionSize = 0;
    bool validInstruction = false;
    QByteArray error(MAX_ERROR_SIZE, 0);
    BASIC_INSTRUCTION_INFO basicInstrInfo;

    // Get selected instruction info (size here)
    DbgDisasmFastAt(mSelectedInstrVa, &basicInstrInfo);
    selectedInstructionSize = basicInstrInfo.size;

    // Get typed in instruction size
    if(!DbgFunctions()->Assemble(this->mSelectedInstrVa, NULL, &typedInstructionSize, editText.toUtf8().constData(), error.data())  || selectedInstructionSize == 0)
    {
        emit mValidateThread->emitInstructionChanged(0, QString(error));
        return;
    }

    // Valid instruction
    validInstruction = true;

    sizeDifference = typedInstructionSize - selectedInstructionSize;

    emit mValidateThread->emitInstructionChanged(sizeDifference, "");
}

void AssembleDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    mValidateThread->stop();
    mValidateThread->wait();
}

void AssembleDialog::textChangedSlot(QString text)
{
    if(ui->checkBoxKeepSize->isChecked())
        mValidateThread->textChanged(text);
}

void AssembleDialog::instructionChangedSlot(dsint sizeDifference, QString error)
{
    if(ui->checkBoxKeepSize->isChecked())
    {
        // If there was an error
        if(error.length())
        {
            this->setKeepSizeLabel("<font color='orange'><b>Instruction decoding error : " + error + "</b></font>");
            return;
        }
        // No error
        else
        {

            // SizeDifference >  0 <=> Typed instruction is bigger
            if(sizeDifference > 0)
            {
                QString message = "<font color='red'><b>Instruction bigger by " + QString::number(sizeDifference);
                if(sizeDifference == 1)
                    message += QString(" byte</b></font>");
                else
                    message += QString(" bytes</b></font>");

                this->setKeepSizeLabel(message);
                this->setOkButtonEnabled(false);
            }
            // SizeDifference < 0 <=> Typed instruction is smaller
            else if(sizeDifference < 0)
            {
                QString message = "<font color='#00cc00'><b>Instruction smaller by " + QString::number(sizeDifference);
                if(sizeDifference == -1)
                    message += QString(" byte</b></font>");
                else
                    message += QString(" bytes</b></font>");

                this->setKeepSizeLabel(message);
                this->setOkButtonEnabled(true);
            }
            // SizeDifference == 0 <=> Both instruction have same size
            else
            {
                QString message = "<font color='#00cc00'><b>Instruction is same size</b></font>";

                this->setKeepSizeLabel(message);
                this->setOkButtonEnabled(true);
            }
        }
    }
}

void AssembleDialog::on_lineEdit_textChanged(const QString & arg1)
{
    editText = arg1;

    if(ui->checkBoxKeepSize->isChecked() && editText.size())
        mValidateThread->start(editText);
}

void AssembleDialog::on_checkBoxKeepSize_clicked(bool checked)
{
    if(checked && editText.size())
    {
        mValidateThread->start();
        mValidateThread->textChanged(ui->lineEdit->text()); // Have to add this or textChanged isn't called inside start()
    }
    else
    {
        mValidateThread->stop();
        mValidateThread->wait();
        ui->labelKeepSize->setText("");
        ui->pushButtonOk->setEnabled(true);
    }
    bKeepSizeChecked = checked;
}

void AssembleDialog::on_checkBoxFillWithNops_clicked(bool checked)
{
    bFillWithNopsChecked = checked;
}

#include "AssembleDialog.h"
#include "ui_AssembleDialog.h"
#include "ValidateExpressionThread.h"
#include <QMessageBox>
#include "Configuration.h"

bool AssembleDialog::bWarningShowedOnce = false;
#define ASSEMBLE_ERROR (-1337)

AssembleDialog::AssembleDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AssembleDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    mSelectedInstrVa = 0;
    bKeepSizeChecked = false;
    bFillWithNopsChecked = false;
    setKeepSizeLabel("");

    mValidateThread = new ValidateExpressionThread(this);
    mValidateThread->setOnExpressionChangedCallback(std::bind(&AssembleDialog::validateInstruction, this, std::placeholders::_1));

    connect(ui->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChangedSlot(QString)));
    connect(mValidateThread, SIGNAL(instructionChanged(dsint, QString)), this, SLOT(instructionChangedSlot(dsint, QString)));
    mValidateThread->start();

    duint setting;
    if(BridgeSettingGetUint("Engine", "Assembler", &setting))
    {
        if(setting == 1 || setting == 2)
            ui->radioAsmjit->setChecked(true);
    }

    Config()->loadWindowGeometry(this);
}

AssembleDialog::~AssembleDialog()
{
    mValidateThread->stop();
    mValidateThread->wait();
    Config()->saveWindowGeometry(this);
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

void AssembleDialog::setSelectedInstrVa(const duint va)
{
    mSelectedInstrVa = va;
}

void AssembleDialog::setOkButtonEnabled(bool enabled)
{
    ui->pushButtonOk->setEnabled(enabled);
}

void AssembleDialog::validateInstruction(QString expression)
{
    //sanitize the expression (just simplifying it by removing excess whitespaces)
    expression = expression.simplified();

    if(!expression.length())
    {
        emit mValidateThread->emitInstructionChanged(ASSEMBLE_ERROR, tr("empty instruction"));
        return;
    }
    //void instructionChanged(bool validInstruction, dsint sizeDifference, QString error)
    dsint sizeDifference = 0;
    int typedInstructionSize = 0;
    int selectedInstructionSize = 0;
    bool validInstruction = false;
    QByteArray error(MAX_ERROR_SIZE, 0);
    QByteArray opcode(16, 0);
    BASIC_INSTRUCTION_INFO basicInstrInfo;

    // Get selected instruction info (size here)
    DbgDisasmFastAt(mSelectedInstrVa, &basicInstrInfo);
    selectedInstructionSize = basicInstrInfo.size;

    // Get typed in instruction size
    if(!DbgFunctions()->Assemble(mSelectedInstrVa, (unsigned char*)opcode.data(), &typedInstructionSize, expression.toUtf8().constData(), error.data())  || selectedInstructionSize == 0)
    {
        emit mValidateThread->emitInstructionChanged(ASSEMBLE_ERROR, QString(error));
        return;
    }

    // Valid instruction
    validInstruction = true;

    sizeDifference = typedInstructionSize - selectedInstructionSize;

    opcode.resize(typedInstructionSize);
    emit mValidateThread->emitInstructionChanged(sizeDifference, opcode.toHex().toUpper());
}

void AssembleDialog::textChangedSlot(QString text)
{
    mValidateThread->textChanged(text);
}

void AssembleDialog::instructionChangedSlot(dsint sizeDifference, QString data)
{
    // If there was an error
    if(sizeDifference == ASSEMBLE_ERROR)
    {
        setKeepSizeLabel(tr("<font color='orange'><b>Instruction encoding error: %1</b></font>").arg(data));
        setOkButtonEnabled(false);
    }
    else if(ui->checkBoxKeepSize->isChecked())
    {
        // SizeDifference >  0 <=> Typed instruction is bigger
        if(sizeDifference > 0)
        {
            QString message = tr("<font color='red'><b>Instruction bigger by %1 %2...</b></font>")
                              .arg(sizeDifference)
                              .arg(sizeDifference == 1 ? tr("byte") : tr("bytes")).append(tr("<br>Bytes: %1").arg(data));

            setKeepSizeLabel(message);
            setOkButtonEnabled(false);
        }
        // SizeDifference < 0 <=> Typed instruction is smaller
        else if(sizeDifference < 0)
        {
            QString message = tr("<font color='#00cc00'><b>Instruction smaller by %1 %2...</b></font>")
                              .arg(-sizeDifference)
                              .arg(sizeDifference == -1 ? tr("byte") : tr("bytes")).append(tr("<br>Bytes: %1").arg(data));

            setKeepSizeLabel(message);
            setOkButtonEnabled(true);
        }
        // SizeDifference == 0 <=> Both instruction have same size
        else
        {
            QString message = tr("<font color='#00cc00'><b>Instruction is same size!</b></font>").append(tr("<br>Bytes: %1").arg(data));

            setKeepSizeLabel(message);
            setOkButtonEnabled(true);
        }
    }
    else
    {
        QString message = tr("<font color='#00cc00'><b>Instruction encoded successfully!</b></font>").append(tr("<br>Bytes: %1").arg(data));

        setKeepSizeLabel(message);
        setOkButtonEnabled(true);
    }
}

void AssembleDialog::on_lineEdit_textChanged(const QString & arg1)
{
    editText = arg1;
}

void AssembleDialog::on_checkBoxKeepSize_clicked(bool checked)
{
    bKeepSizeChecked = checked;
    mValidateThread->additionalStateChanged();
}

void AssembleDialog::on_checkBoxFillWithNops_clicked(bool checked)
{
    bFillWithNopsChecked = checked;
}

void AssembleDialog::on_radioXEDParse_clicked()
{
    BridgeSettingSetUint("Engine", "Assembler", 0);
    DbgSettingsUpdated();
    validateInstruction(ui->lineEdit->text());
}

void AssembleDialog::on_radioAsmjit_clicked()
{
    BridgeSettingSetUint("Engine", "Assembler", 2);
    DbgSettingsUpdated();
    validateInstruction(ui->lineEdit->text());
}

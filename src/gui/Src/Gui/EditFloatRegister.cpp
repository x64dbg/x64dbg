#include "EditFloatRegister.h"
#include "ui_EditFloatRegister.h"
#include "Bridge.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include "Configuration.h"

/**
 * @brief       Initialize EditFloatRegister dialog
 *
 * @param[in]   RegisterSize    The register size. 128 stands for XMM register, 256 stands for YMM register,
 *                              512 stands for ZMM register.
 *
 * @param[in]   parent          The parent of this dialog.
 *
 * @return      Nothing.
 */

EditFloatRegister::EditFloatRegister(int RegisterSize, QWidget* parent) :
    QDialog(parent), hexValidate(this), RegSize(RegisterSize),
    signedShortValidator(LongLongValidator::DataType::SignedShort, this),
    unsignedShortValidator(LongLongValidator::DataType::UnsignedShort, this),
    signedLongValidator(LongLongValidator::DataType::SignedLong, this),
    unsignedLongValidator(LongLongValidator::DataType::UnsignedLong, this),
    signedLongLongValidator(LongLongValidator::DataType::SignedLongLong, this),
    unsignedLongLongValidator(LongLongValidator::DataType::UnsignedLongLong, this),
    doubleValidator(this),
    ui(new Ui::EditFloatRegister)
{
    memset(Data, 0, sizeof(Data));
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    switch(RegisterSize)
    {
    case 128:
        hideUpperPart();
        ui->labelLowRegister->setText(QString("XMM:"));
        break;
    case 64:
        hideUpperPart();
        hideNonMMXPart();
        ui->hexEdit_2->setMaxLength(16);
        ui->labelLowRegister->setText(QString("MM:"));
        break;
    case 256:
        break;
    case 512:
    default:
        GuiAddLogMessage(tr("Error, register size %1 is not supported.\n").arg(RegisterSize).toUtf8().constData());
        break;
    }
    setFixedWidth(width());
    adjustSize();

    connect(ui->hexEdit, SIGNAL(textEdited(QString)), this, SLOT(editingHex1FinishedSlot(QString)));
    ui->hexEdit->setValidator(&hexValidate);
    connect(ui->hexEdit_2, SIGNAL(textEdited(QString)), this, SLOT(editingHex2FinishedSlot(QString)));
    ui->hexEdit_2->setValidator(&hexValidate);
    editingModeChangedSlot(false);
    connect(ui->radioHex, SIGNAL(toggled(bool)), this, SLOT(editingModeChangedSlot(bool)));
    connect(ui->radioSigned, SIGNAL(toggled(bool)), this, SLOT(editingModeChangedSlot(bool)));
    connect(ui->radioUnsigned, SIGNAL(toggled(bool)), this, SLOT(editingModeChangedSlot(bool)));
    connect(ui->shortEdit0_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerShort0FinishedSlot(QString)));
    connect(ui->shortEdit1_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerShort1FinishedSlot(QString)));
    connect(ui->shortEdit2_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerShort2FinishedSlot(QString)));
    connect(ui->shortEdit3_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerShort3FinishedSlot(QString)));
    if(RegisterSize > 64)
    {
        connect(ui->shortEdit4_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerShort4FinishedSlot(QString)));
        connect(ui->shortEdit5_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerShort5FinishedSlot(QString)));
        connect(ui->shortEdit6_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerShort6FinishedSlot(QString)));
        connect(ui->shortEdit7_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerShort7FinishedSlot(QString)));
        if(RegisterSize > 128)
        {
            connect(ui->shortEdit0, SIGNAL(textEdited(QString)), this, SLOT(editingUpperShort0FinishedSlot(QString)));
            connect(ui->shortEdit1, SIGNAL(textEdited(QString)), this, SLOT(editingUpperShort1FinishedSlot(QString)));
            connect(ui->shortEdit2, SIGNAL(textEdited(QString)), this, SLOT(editingUpperShort2FinishedSlot(QString)));
            connect(ui->shortEdit3, SIGNAL(textEdited(QString)), this, SLOT(editingUpperShort3FinishedSlot(QString)));
            connect(ui->shortEdit4, SIGNAL(textEdited(QString)), this, SLOT(editingUpperShort4FinishedSlot(QString)));
            connect(ui->shortEdit5, SIGNAL(textEdited(QString)), this, SLOT(editingUpperShort5FinishedSlot(QString)));
            connect(ui->shortEdit6, SIGNAL(textEdited(QString)), this, SLOT(editingUpperShort6FinishedSlot(QString)));
            connect(ui->shortEdit7, SIGNAL(textEdited(QString)), this, SLOT(editingUpperShort7FinishedSlot(QString)));
        }
    }
    connect(ui->longEdit0_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerLong0FinishedSlot(QString)));
    connect(ui->longEdit1_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerLong1FinishedSlot(QString)));
    if(RegisterSize > 64)
    {
        connect(ui->longEdit2_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerLong2FinishedSlot(QString)));
        connect(ui->longEdit3_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerLong3FinishedSlot(QString)));
        if(RegisterSize > 128)
        {
            connect(ui->longEdit0, SIGNAL(textEdited(QString)), this, SLOT(editingUpperLong0FinishedSlot(QString)));
            connect(ui->longEdit1, SIGNAL(textEdited(QString)), this, SLOT(editingUpperLong1FinishedSlot(QString)));
            connect(ui->longEdit2, SIGNAL(textEdited(QString)), this, SLOT(editingUpperLong2FinishedSlot(QString)));
            connect(ui->longEdit3, SIGNAL(textEdited(QString)), this, SLOT(editingUpperLong3FinishedSlot(QString)));
        }
    }
    connect(ui->floatEdit0_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerFloat0FinishedSlot(QString)));
    connect(ui->floatEdit1_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerFloat1FinishedSlot(QString)));
    if(RegisterSize > 64)
    {
        connect(ui->floatEdit2_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerFloat2FinishedSlot(QString)));
        connect(ui->floatEdit3_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerFloat3FinishedSlot(QString)));
        if(RegisterSize > 128)
        {
            connect(ui->floatEdit0, SIGNAL(textEdited(QString)), this, SLOT(editingUpperFloat0FinishedSlot(QString)));
            connect(ui->floatEdit1, SIGNAL(textEdited(QString)), this, SLOT(editingUpperFloat1FinishedSlot(QString)));
            connect(ui->floatEdit2, SIGNAL(textEdited(QString)), this, SLOT(editingUpperFloat2FinishedSlot(QString)));
            connect(ui->floatEdit3, SIGNAL(textEdited(QString)), this, SLOT(editingUpperFloat3FinishedSlot(QString)));
        }
    }
    if(RegisterSize > 64)
    {
        connect(ui->doubleEdit0_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerDouble0FinishedSlot(QString)));
        connect(ui->doubleEdit1_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerDouble1FinishedSlot(QString)));
        if(RegisterSize > 128)
        {
            connect(ui->doubleEdit0, SIGNAL(textEdited(QString)), this, SLOT(editingUpperDouble0FinishedSlot(QString)));
            connect(ui->doubleEdit1, SIGNAL(textEdited(QString)), this, SLOT(editingUpperDouble1FinishedSlot(QString)));
        }
        connect(ui->longLongEdit0_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerLongLong0FinishedSlot(QString)));
        connect(ui->longLongEdit1_2, SIGNAL(textEdited(QString)), this, SLOT(editingLowerLongLong1FinishedSlot(QString)));
        if(RegisterSize > 128)
        {
            connect(ui->longLongEdit0, SIGNAL(textEdited(QString)), this, SLOT(editingUpperLongLong0FinishedSlot(QString)));
            connect(ui->longLongEdit1, SIGNAL(textEdited(QString)), this, SLOT(editingUpperLongLong1FinishedSlot(QString)));
        }
    }
    ui->floatEdit0_2->setValidator(&doubleValidator);
    ui->floatEdit1_2->setValidator(&doubleValidator);
    if(RegisterSize > 64)
    {
        ui->floatEdit2_2->setValidator(&doubleValidator);
        ui->floatEdit3_2->setValidator(&doubleValidator);
        if(RegisterSize > 128)
        {
            ui->floatEdit0->setValidator(&doubleValidator);
            ui->floatEdit1->setValidator(&doubleValidator);
            ui->floatEdit2->setValidator(&doubleValidator);
            ui->floatEdit3->setValidator(&doubleValidator);
        }
    }
    if(RegisterSize > 64)
    {
        ui->doubleEdit0_2->setValidator(&doubleValidator);
        ui->doubleEdit1_2->setValidator(&doubleValidator);
        if(RegisterSize > 128)
        {
            ui->doubleEdit0->setValidator(&doubleValidator);
            ui->doubleEdit1->setValidator(&doubleValidator);
        }
    }
}

void EditFloatRegister::hideUpperPart()
{
    ui->line->hide();
    ui->labelH0->hide();
    ui->labelH1->hide();
    ui->labelH2->hide();
    ui->labelH3->hide();
    ui->labelH4->hide();
    ui->labelH5->hide();
    ui->labelH6->hide();
    ui->labelH7->hide();
    ui->labelH8->hide();
    ui->labelH9->hide();
    ui->labelHA->hide();
    ui->labelHB->hide();
    ui->labelHC->hide();
    ui->labelHD->hide();
    ui->labelHE->hide();
    ui->hexEdit->hide();
    ui->shortEdit0->hide();
    ui->shortEdit1->hide();
    ui->shortEdit2->hide();
    ui->shortEdit3->hide();
    ui->shortEdit4->hide();
    ui->shortEdit5->hide();
    ui->shortEdit6->hide();
    ui->shortEdit7->hide();
    ui->longEdit0->hide();
    ui->longEdit1->hide();
    ui->longEdit2->hide();
    ui->longEdit3->hide();
    ui->floatEdit0->hide();
    ui->floatEdit1->hide();
    ui->floatEdit2->hide();
    ui->floatEdit3->hide();
    ui->doubleEdit0->hide();
    ui->doubleEdit1->hide();
    ui->longLongEdit0->hide();
    ui->longLongEdit1->hide();
}

void EditFloatRegister::hideNonMMXPart()
{
    ui->labelL4->hide();
    ui->labelL5->hide();
    ui->labelL6->hide();
    ui->labelL7->hide();
    ui->labelLC->hide();
    ui->labelLD->hide();
    ui->doubleEdit0_2->hide();
    ui->doubleEdit1_2->hide();
    ui->longLongEdit0_2->hide();
    ui->longLongEdit1_2->hide();
    ui->shortEdit4_2->hide();
    ui->shortEdit5_2->hide();
    ui->shortEdit6_2->hide();
    ui->shortEdit7_2->hide();
    ui->longEdit2_2->hide();
    ui->longEdit3_2->hide();
    ui->floatEdit2_2->hide();
    ui->floatEdit3_2->hide();
}

/**
 * @brief                    Load register data into the dialog
 * @param[in] RegisterData   the data to be loaded. It must be at lease the same size as the size specified in RegisterSize
 * @return    Nothing.
 */
void EditFloatRegister::loadData(char* RegisterData)
{
    memcpy(Data, RegisterData, RegSize / 8);
    reloadDataLow();
    reloadDataHigh();
}

/**
 * @brief    Get the register data from the dialog
 * @return   The output buffer.
 */
const char* EditFloatRegister::getData()
{
    return Data;
}

void EditFloatRegister::selectAllText()
{
    ui->hexEdit_2->setFocus();
    ui->hexEdit_2->selectAll();
}

/**
 * @brief reloads the lower 128-bit of data of the dialog
 */
void EditFloatRegister::reloadDataLow()
{
    if(mutex == nullptr)
        mutex = this;
    int maxBytes;
    if(RegSize >= 128)
        maxBytes = 16;
    else
        maxBytes = RegSize / 8;
    if(mutex != ui->hexEdit_2)
    {
        if(ConfigBool("Gui", "FpuRegistersLittleEndian"))
            ui->hexEdit_2->setText(QString(QByteArray(Data, maxBytes).toHex()).toUpper());
        else
            ui->hexEdit_2->setText(QString(ByteReverse(QByteArray(Data, maxBytes)).toHex()).toUpper());
    }
    reloadLongData(*ui->longEdit0_2, Data);
    reloadLongData(*ui->longEdit1_2, Data + 4);
    if(RegSize > 64)
    {
        reloadLongData(*ui->longEdit2_2, Data + 8);
        reloadLongData(*ui->longEdit3_2, Data + 12);
    }
    reloadShortData(*ui->shortEdit0_2, Data);
    reloadShortData(*ui->shortEdit1_2, Data + 2);
    reloadShortData(*ui->shortEdit2_2, Data + 4);
    reloadShortData(*ui->shortEdit3_2, Data + 6);
    if(RegSize > 64)
    {
        reloadShortData(*ui->shortEdit4_2, Data + 8);
        reloadShortData(*ui->shortEdit5_2, Data + 10);
        reloadShortData(*ui->shortEdit6_2, Data + 12);
        reloadShortData(*ui->shortEdit7_2, Data + 14);
    }
    reloadFloatData(*ui->floatEdit0_2, Data);
    reloadFloatData(*ui->floatEdit1_2, Data + 4);
    if(RegSize > 64)
    {
        reloadFloatData(*ui->floatEdit2_2, Data + 8);
        reloadFloatData(*ui->floatEdit3_2, Data + 12);
        reloadDoubleData(*ui->doubleEdit0_2, Data);
        reloadDoubleData(*ui->doubleEdit1_2, Data + 8);
        reloadLongLongData(*ui->longLongEdit0_2, Data);
        reloadLongLongData(*ui->longLongEdit1_2, Data + 8);
    }
    mutex = nullptr;
}

/**
 * @brief reloads the upper 128-bit of data of the dialog
 */
void EditFloatRegister::reloadDataHigh()
{
    if(mutex == nullptr)
        mutex = this;
    if(mutex != ui->hexEdit)
    {
        if(ConfigBool("Gui", "FpuRegistersLittleEndian"))
            ui->hexEdit->setText(QString(QByteArray(Data + 16, 16).toHex()).toUpper());
        else
            ui->hexEdit->setText(QString(ByteReverse(QByteArray(Data + 16, 16)).toHex()).toUpper());
    }
    reloadLongData(*ui->longEdit0, Data + 16);
    reloadLongData(*ui->longEdit1, Data + 20);
    reloadLongData(*ui->longEdit2, Data + 24);
    reloadLongData(*ui->longEdit3, Data + 28);
    reloadShortData(*ui->shortEdit0, Data + 16);
    reloadShortData(*ui->shortEdit1, Data + 18);
    reloadShortData(*ui->shortEdit2, Data + 20);
    reloadShortData(*ui->shortEdit3, Data + 22);
    reloadShortData(*ui->shortEdit4, Data + 24);
    reloadShortData(*ui->shortEdit5, Data + 26);
    reloadShortData(*ui->shortEdit6, Data + 28);
    reloadShortData(*ui->shortEdit7, Data + 30);
    reloadFloatData(*ui->floatEdit0, Data + 16);
    reloadFloatData(*ui->floatEdit1, Data + 20);
    reloadFloatData(*ui->floatEdit2, Data + 24);
    reloadFloatData(*ui->floatEdit3, Data + 28);
    reloadDoubleData(*ui->doubleEdit0, Data + 16);
    reloadDoubleData(*ui->doubleEdit1, Data + 24);
    reloadLongLongData(*ui->longLongEdit0, Data + 16);
    reloadLongLongData(*ui->longLongEdit1, Data + 24);
    mutex = nullptr;
}

void EditFloatRegister::reloadShortData(QLineEdit & txtbox, char* Data)
{
    if(mutex != &txtbox)
    {
        if(ui->radioHex->isChecked())
            txtbox.setText(QString().number((int) * (unsigned short*)Data, 16).toUpper());
        else if(ui->radioSigned->isChecked())
            txtbox.setText(QString().number((int) * (short*)Data));
        else
            txtbox.setText(QString().number((unsigned int) * (unsigned short*)Data));
    }
}

void EditFloatRegister::reloadLongData(QLineEdit & txtbox, char* Data)
{
    if(mutex != &txtbox)
    {
        if(ui->radioHex->isChecked())
            txtbox.setText(QString().number(*(unsigned int*)Data, 16).toUpper());
        else if(ui->radioSigned->isChecked())
            txtbox.setText(QString().number(*(int*)Data));
        else
            txtbox.setText(QString().number(*(unsigned int*)Data));
    }
}

void EditFloatRegister::reloadFloatData(QLineEdit & txtbox, char* Data)
{
    if(mutex != &txtbox)
    {
        txtbox.setText(ToFloatString(Data));
    }
}

void EditFloatRegister::reloadDoubleData(QLineEdit & txtbox, char* Data)
{
    if(mutex != &txtbox)
    {
        txtbox.setText(ToDoubleString(Data));
    }
}

void EditFloatRegister::reloadLongLongData(QLineEdit & txtbox, char* Data)
{
    if(mutex != &txtbox)
    {
        if(ui->radioHex->isChecked())
            txtbox.setText(QString().number(*(unsigned long long*)Data, 16).toUpper());
        else if(ui->radioSigned->isChecked())
            txtbox.setText(QString().number(*(long long*)Data));
        else
            txtbox.setText(QString().number(*(unsigned long long*)Data));
    }
}

void EditFloatRegister::editingModeChangedSlot(bool arg)
{
    Q_UNUSED(arg);
    if(ui->radioHex->isChecked())
    {
        ui->shortEdit0_2->setMaxLength(4);
        ui->shortEdit1_2->setMaxLength(4);
        ui->shortEdit2_2->setMaxLength(4);
        ui->shortEdit3_2->setMaxLength(4);
        if(RegSize > 64)
        {
            ui->shortEdit4_2->setMaxLength(4);
            ui->shortEdit5_2->setMaxLength(4);
            ui->shortEdit6_2->setMaxLength(4);
            ui->shortEdit7_2->setMaxLength(4);
            if(RegSize > 128)
            {
                ui->shortEdit0->setMaxLength(4);
                ui->shortEdit1->setMaxLength(4);
                ui->shortEdit2->setMaxLength(4);
                ui->shortEdit3->setMaxLength(4);
                ui->shortEdit4->setMaxLength(4);
                ui->shortEdit5->setMaxLength(4);
                ui->shortEdit6->setMaxLength(4);
                ui->shortEdit7->setMaxLength(4);
            }
        }
        ui->longEdit0_2->setMaxLength(8);
        ui->longEdit1_2->setMaxLength(8);
        if(RegSize > 64)
        {
            ui->longEdit2_2->setMaxLength(8);
            ui->longEdit3_2->setMaxLength(8);
            if(RegSize > 128)
            {
                ui->longEdit0->setMaxLength(8);
                ui->longEdit1->setMaxLength(8);
                ui->longEdit2->setMaxLength(8);
                ui->longEdit3->setMaxLength(8);
                ui->longLongEdit0->setMaxLength(16);
                ui->longLongEdit1->setMaxLength(16);
            }
            ui->longLongEdit0_2->setMaxLength(16);
            ui->longLongEdit1_2->setMaxLength(16);
        }
        ui->shortEdit0_2->setValidator(&hexValidate);
        ui->shortEdit1_2->setValidator(&hexValidate);
        ui->shortEdit2_2->setValidator(&hexValidate);
        ui->shortEdit3_2->setValidator(&hexValidate);
        if(RegSize > 64)
        {
            ui->shortEdit4_2->setValidator(&hexValidate);
            ui->shortEdit5_2->setValidator(&hexValidate);
            ui->shortEdit6_2->setValidator(&hexValidate);
            ui->shortEdit7_2->setValidator(&hexValidate);
            if(RegSize > 128)
            {
                ui->shortEdit0->setValidator(&hexValidate);
                ui->shortEdit1->setValidator(&hexValidate);
                ui->shortEdit2->setValidator(&hexValidate);
                ui->shortEdit3->setValidator(&hexValidate);
                ui->shortEdit4->setValidator(&hexValidate);
                ui->shortEdit5->setValidator(&hexValidate);
                ui->shortEdit6->setValidator(&hexValidate);
                ui->shortEdit7->setValidator(&hexValidate);
            }
        }
        ui->longEdit0_2->setValidator(&hexValidate);
        ui->longEdit1_2->setValidator(&hexValidate);
        if(RegSize > 64)
        {
            ui->longEdit2_2->setValidator(&hexValidate);
            ui->longEdit3_2->setValidator(&hexValidate);
            if(RegSize > 128)
            {
                ui->longEdit0->setValidator(&hexValidate);
                ui->longEdit1->setValidator(&hexValidate);
                ui->longEdit2->setValidator(&hexValidate);
                ui->longEdit3->setValidator(&hexValidate);
                ui->longLongEdit0->setValidator(&hexValidate);
                ui->longLongEdit1->setValidator(&hexValidate);
            }
            ui->longLongEdit0_2->setValidator(&hexValidate);
            ui->longLongEdit1_2->setValidator(&hexValidate);
        }
    }
    else if(ui->radioSigned->isChecked())
    {
        ui->shortEdit0_2->setMaxLength(6);
        ui->shortEdit1_2->setMaxLength(6);
        ui->shortEdit2_2->setMaxLength(6);
        ui->shortEdit3_2->setMaxLength(6);
        ui->shortEdit4_2->setMaxLength(6);
        ui->shortEdit5_2->setMaxLength(6);
        ui->shortEdit6_2->setMaxLength(6);
        ui->shortEdit7_2->setMaxLength(6);
        ui->shortEdit0->setMaxLength(6);
        ui->shortEdit1->setMaxLength(6);
        ui->shortEdit2->setMaxLength(6);
        ui->shortEdit3->setMaxLength(6);
        ui->shortEdit4->setMaxLength(6);
        ui->shortEdit5->setMaxLength(6);
        ui->shortEdit6->setMaxLength(6);
        ui->shortEdit7->setMaxLength(6);
        ui->longEdit0_2->setMaxLength(12);
        ui->longEdit1_2->setMaxLength(12);
        ui->longEdit2_2->setMaxLength(12);
        ui->longEdit3_2->setMaxLength(12);
        ui->longEdit0->setMaxLength(12);
        ui->longEdit1->setMaxLength(12);
        ui->longEdit2->setMaxLength(12);
        ui->longEdit3->setMaxLength(12);
        ui->longLongEdit0->setMaxLength(64);
        ui->longLongEdit1->setMaxLength(64);
        ui->longLongEdit0_2->setMaxLength(64);
        ui->longLongEdit1_2->setMaxLength(64);
        ui->shortEdit0_2->setValidator(&signedShortValidator);
        ui->shortEdit1_2->setValidator(&signedShortValidator);
        ui->shortEdit2_2->setValidator(&signedShortValidator);
        ui->shortEdit3_2->setValidator(&signedShortValidator);
        ui->shortEdit4_2->setValidator(&signedShortValidator);
        ui->shortEdit5_2->setValidator(&signedShortValidator);
        ui->shortEdit6_2->setValidator(&signedShortValidator);
        ui->shortEdit7_2->setValidator(&signedShortValidator);
        ui->shortEdit0->setValidator(&signedShortValidator);
        ui->shortEdit1->setValidator(&signedShortValidator);
        ui->shortEdit2->setValidator(&signedShortValidator);
        ui->shortEdit3->setValidator(&signedShortValidator);
        ui->shortEdit4->setValidator(&signedShortValidator);
        ui->shortEdit5->setValidator(&signedShortValidator);
        ui->shortEdit6->setValidator(&signedShortValidator);
        ui->shortEdit7->setValidator(&signedShortValidator);
        ui->longEdit0_2->setValidator(&signedLongValidator);
        ui->longEdit1_2->setValidator(&signedLongValidator);
        ui->longEdit2_2->setValidator(&signedLongValidator);
        ui->longEdit3_2->setValidator(&signedLongValidator);
        ui->longEdit0->setValidator(&signedLongValidator);
        ui->longEdit1->setValidator(&signedLongValidator);
        ui->longEdit2->setValidator(&signedLongValidator);
        ui->longEdit3->setValidator(&signedLongValidator);
        ui->longLongEdit0->setValidator(&signedLongLongValidator);
        ui->longLongEdit1->setValidator(&signedLongLongValidator);
        ui->longLongEdit0_2->setValidator(&signedLongLongValidator);
        ui->longLongEdit1_2->setValidator(&signedLongLongValidator);
    }
    else
    {
        ui->shortEdit0_2->setMaxLength(6);
        ui->shortEdit1_2->setMaxLength(6);
        ui->shortEdit2_2->setMaxLength(6);
        ui->shortEdit3_2->setMaxLength(6);
        ui->shortEdit4_2->setMaxLength(6);
        ui->shortEdit5_2->setMaxLength(6);
        ui->shortEdit6_2->setMaxLength(6);
        ui->shortEdit7_2->setMaxLength(6);
        ui->shortEdit0->setMaxLength(6);
        ui->shortEdit1->setMaxLength(6);
        ui->shortEdit2->setMaxLength(6);
        ui->shortEdit3->setMaxLength(6);
        ui->shortEdit4->setMaxLength(6);
        ui->shortEdit5->setMaxLength(6);
        ui->shortEdit6->setMaxLength(6);
        ui->shortEdit7->setMaxLength(6);
        ui->longEdit0_2->setMaxLength(12);
        ui->longEdit1_2->setMaxLength(12);
        ui->longEdit2_2->setMaxLength(12);
        ui->longEdit3_2->setMaxLength(12);
        ui->longEdit0->setMaxLength(12);
        ui->longEdit1->setMaxLength(12);
        ui->longEdit2->setMaxLength(12);
        ui->longEdit3->setMaxLength(12);
        ui->shortEdit0_2->setValidator(&unsignedShortValidator);
        ui->shortEdit1_2->setValidator(&unsignedShortValidator);
        ui->shortEdit2_2->setValidator(&unsignedShortValidator);
        ui->shortEdit3_2->setValidator(&unsignedShortValidator);
        ui->shortEdit4_2->setValidator(&unsignedShortValidator);
        ui->shortEdit5_2->setValidator(&unsignedShortValidator);
        ui->shortEdit6_2->setValidator(&unsignedShortValidator);
        ui->shortEdit7_2->setValidator(&unsignedShortValidator);
        ui->shortEdit0->setValidator(&unsignedShortValidator);
        ui->shortEdit1->setValidator(&unsignedShortValidator);
        ui->shortEdit2->setValidator(&unsignedShortValidator);
        ui->shortEdit3->setValidator(&unsignedShortValidator);
        ui->shortEdit4->setValidator(&unsignedShortValidator);
        ui->shortEdit5->setValidator(&unsignedShortValidator);
        ui->shortEdit6->setValidator(&unsignedShortValidator);
        ui->shortEdit7->setValidator(&unsignedShortValidator);
        ui->longEdit0_2->setValidator(&unsignedLongValidator);
        ui->longEdit1_2->setValidator(&unsignedLongValidator);
        ui->longEdit2_2->setValidator(&unsignedLongValidator);
        ui->longEdit3_2->setValidator(&unsignedLongValidator);
        ui->longEdit0->setValidator(&unsignedLongValidator);
        ui->longEdit1->setValidator(&unsignedLongValidator);
        ui->longEdit2->setValidator(&unsignedLongValidator);
        ui->longEdit3->setValidator(&unsignedLongValidator);
        ui->longLongEdit0->setValidator(&unsignedLongLongValidator);
        ui->longLongEdit1->setValidator(&unsignedLongLongValidator);
        ui->longLongEdit0_2->setValidator(&unsignedLongLongValidator);
        ui->longLongEdit1_2->setValidator(&unsignedLongLongValidator);
    }
    reloadDataLow();
    if(RegSize > 128)
        reloadDataHigh();
}

/**
 * @brief Desturctor of EditFloatRegister
 * @return nothing
 */

EditFloatRegister::~EditFloatRegister()
{
    delete ui;
}

/**
 * @brief     The higher part of the YMM register (or XMM register) is modified
 * @param arg the new text
 */
void EditFloatRegister::editingHex1FinishedSlot(QString arg)
{
    mutex = sender();
    QString filled(arg.toUpper());
    if(ConfigBool("Gui", "FpuRegistersLittleEndian"))
    {
        filled.append(QString(32 - filled.length(), QChar('0')));
        for(int i = 0; i < 16; i++)
            Data[i + 16] = filled.mid(i * 2, 2).toInt(0, 16);
    }
    else
    {
        filled.prepend(QString(32 - filled.length(), QChar('0')));
        for(int i = 0; i < 16; i++)
            Data[i + 16] = filled.mid(30 - i * 2, 2).toInt(0, 16);
    }
    reloadDataHigh();
}

/**
 * @brief     The lower part of the YMM register (or XMM register) is modified
 * @param arg the new text
 */
void EditFloatRegister::editingHex2FinishedSlot(QString arg)
{
    mutex = sender();
    QString filled(arg.toUpper());
    int maxBytes;
    if(RegSize >= 128)
        maxBytes = 16;
    else
        maxBytes = RegSize / 8;
    if(ConfigBool("Gui", "FpuRegistersLittleEndian"))
    {
        filled.append(QString(maxBytes * 2 - filled.length(), QChar('0')));
        for(int i = 0; i < maxBytes; i++)
            Data[i] = filled.mid(i * 2, 2).toInt(0, 16);
    }
    else
    {
        filled.prepend(QString(maxBytes * 2 - filled.length(), QChar('0')));
        for(int i = 0; i < maxBytes; i++)
            Data[i] = filled.mid((maxBytes - i - 1) * 2, 2).toInt(0, 16);
    }
    reloadDataLow();
}

void EditFloatRegister::editingShortFinishedSlot(size_t offset, QString arg)
{
    mutex = sender();
    if(ui->radioHex->isChecked())
        *(unsigned short*)(Data + offset) = arg.toUShort(0, 16);
    else if(ui->radioSigned->isChecked())
        *(short*)(Data + offset) = arg.toShort();
    else
        *(unsigned short*)(Data + offset) = arg.toUShort();
    offset < 16 ? reloadDataLow() : reloadDataHigh();
}
void EditFloatRegister::editingLongFinishedSlot(size_t offset, QString arg)
{
    mutex = sender();
    if(ui->radioHex->isChecked())
        *(unsigned int*)(Data + offset) = arg.toUInt(0, 16);
    else if(ui->radioSigned->isChecked())
        *(int*)(Data + offset) = arg.toInt();
    else
        *(unsigned int*)(Data + offset) = arg.toUInt();
    offset < 16 ? reloadDataLow() : reloadDataHigh();
}
void EditFloatRegister::editingFloatFinishedSlot(size_t offset, QString arg)
{
    mutex = sender();
    bool ok;
    float data = arg.toFloat(&ok);
    if(ok)
        *(float*)(Data + offset) = data;
    offset < 16 ? reloadDataLow() : reloadDataHigh();
}
void EditFloatRegister::editingDoubleFinishedSlot(size_t offset, QString arg)
{
    mutex = sender();
    bool ok;
    double data = arg.toDouble(&ok);
    if(ok)
        *(double*)(Data + offset) = data;
    offset < 16 ? reloadDataLow() : reloadDataHigh();
}
void EditFloatRegister::editingLongLongFinishedSlot(size_t offset, QString arg)
{
    mutex = sender();
    if(ui->radioHex->isChecked())
        *(unsigned long long*)(Data + offset) = arg.toULongLong(0, 16);
    else if(ui->radioSigned->isChecked())
        *(long long*)(Data + offset) = arg.toLongLong();
    else
        *(unsigned long long*)(Data + offset) = arg.toULongLong();
    offset < 16 ? reloadDataLow() : reloadDataHigh();
}

void EditFloatRegister::editingLowerShort0FinishedSlot(QString arg)
{
    editingShortFinishedSlot(0 * 2, arg);
}
void EditFloatRegister::editingLowerShort1FinishedSlot(QString arg)
{
    editingShortFinishedSlot(1 * 2, arg);
}
void EditFloatRegister::editingLowerShort2FinishedSlot(QString arg)
{
    editingShortFinishedSlot(2 * 2, arg);
}
void EditFloatRegister::editingLowerShort3FinishedSlot(QString arg)
{
    editingShortFinishedSlot(3 * 2, arg);
}
void EditFloatRegister::editingLowerShort4FinishedSlot(QString arg)
{
    editingShortFinishedSlot(4 * 2, arg);
}
void EditFloatRegister::editingLowerShort5FinishedSlot(QString arg)
{
    editingShortFinishedSlot(5 * 2, arg);
}
void EditFloatRegister::editingLowerShort6FinishedSlot(QString arg)
{
    editingShortFinishedSlot(6 * 2, arg);
}
void EditFloatRegister::editingLowerShort7FinishedSlot(QString arg)
{
    editingShortFinishedSlot(7 * 2, arg);
}
void EditFloatRegister::editingUpperShort0FinishedSlot(QString arg)
{
    editingShortFinishedSlot(16 + 0 * 2, arg);
}
void EditFloatRegister::editingUpperShort1FinishedSlot(QString arg)
{
    editingShortFinishedSlot(16 + 1 * 2, arg);
}
void EditFloatRegister::editingUpperShort2FinishedSlot(QString arg)
{
    editingShortFinishedSlot(16 + 2 * 2, arg);
}
void EditFloatRegister::editingUpperShort3FinishedSlot(QString arg)
{
    editingShortFinishedSlot(16 + 3 * 2, arg);
}
void EditFloatRegister::editingUpperShort4FinishedSlot(QString arg)
{
    editingShortFinishedSlot(16 + 4 * 2, arg);
}
void EditFloatRegister::editingUpperShort5FinishedSlot(QString arg)
{
    editingShortFinishedSlot(16 + 5 * 2, arg);
}
void EditFloatRegister::editingUpperShort6FinishedSlot(QString arg)
{
    editingShortFinishedSlot(16 + 6 * 2, arg);
}
void EditFloatRegister::editingUpperShort7FinishedSlot(QString arg)
{
    editingShortFinishedSlot(16 + 7 * 2, arg);
}
void EditFloatRegister::editingLowerLong0FinishedSlot(QString arg)
{
    editingLongFinishedSlot(0 * 4, arg);
}
void EditFloatRegister::editingLowerLong1FinishedSlot(QString arg)
{
    editingLongFinishedSlot(1 * 4, arg);
}
void EditFloatRegister::editingLowerLong2FinishedSlot(QString arg)
{
    editingLongFinishedSlot(2 * 4, arg);
}
void EditFloatRegister::editingLowerLong3FinishedSlot(QString arg)
{
    editingLongFinishedSlot(3 * 4, arg);
}
void EditFloatRegister::editingUpperLong0FinishedSlot(QString arg)
{
    editingLongFinishedSlot(16 + 0 * 4, arg);
}
void EditFloatRegister::editingUpperLong1FinishedSlot(QString arg)
{
    editingLongFinishedSlot(16 + 1 * 4, arg);
}
void EditFloatRegister::editingUpperLong2FinishedSlot(QString arg)
{
    editingLongFinishedSlot(16 + 2 * 4, arg);
}
void EditFloatRegister::editingUpperLong3FinishedSlot(QString arg)
{
    editingLongFinishedSlot(16 + 3 * 4, arg);
}
void EditFloatRegister::editingLowerFloat0FinishedSlot(QString arg)
{
    editingFloatFinishedSlot(0 * 4, arg);
}
void EditFloatRegister::editingLowerFloat1FinishedSlot(QString arg)
{
    editingFloatFinishedSlot(1 * 4, arg);
}
void EditFloatRegister::editingLowerFloat2FinishedSlot(QString arg)
{
    editingFloatFinishedSlot(2 * 4, arg);
}
void EditFloatRegister::editingLowerFloat3FinishedSlot(QString arg)
{
    editingFloatFinishedSlot(3 * 4, arg);
}
void EditFloatRegister::editingUpperFloat0FinishedSlot(QString arg)
{
    editingFloatFinishedSlot(16 + 0 * 4, arg);
}
void EditFloatRegister::editingUpperFloat1FinishedSlot(QString arg)
{
    editingFloatFinishedSlot(16 + 1 * 4, arg);
}
void EditFloatRegister::editingUpperFloat2FinishedSlot(QString arg)
{
    editingFloatFinishedSlot(16 + 2 * 4, arg);
}
void EditFloatRegister::editingUpperFloat3FinishedSlot(QString arg)
{
    editingFloatFinishedSlot(16 + 3 * 4, arg);
}
void EditFloatRegister::editingLowerDouble0FinishedSlot(QString arg)
{
    editingDoubleFinishedSlot(0 * 8, arg);
}
void EditFloatRegister::editingLowerDouble1FinishedSlot(QString arg)
{
    editingDoubleFinishedSlot(1 * 8, arg);
}
void EditFloatRegister::editingUpperDouble0FinishedSlot(QString arg)
{
    editingDoubleFinishedSlot(16 + 0 * 8, arg);
}
void EditFloatRegister::editingUpperDouble1FinishedSlot(QString arg)
{
    editingDoubleFinishedSlot(16 + 1 * 8, arg);
}
void EditFloatRegister::editingLowerLongLong0FinishedSlot(QString arg)
{
    editingLongLongFinishedSlot(0 * 8, arg);
}
void EditFloatRegister::editingLowerLongLong1FinishedSlot(QString arg)
{
    editingLongLongFinishedSlot(1 * 8, arg);
}
void EditFloatRegister::editingUpperLongLong0FinishedSlot(QString arg)
{
    editingLongLongFinishedSlot(16 + 0 * 8, arg);
}
void EditFloatRegister::editingUpperLongLong1FinishedSlot(QString arg)
{
    editingLongLongFinishedSlot(16 + 1 * 8, arg);
}

#include "DataCopyDialog.h"
#include "ui_DataCopyDialog.h"
#include "Bridge.h"

DataCopyDialog::DataCopyDialog(const QVector<byte_t>* data, QWidget* parent) : QDialog(parent), ui(new Ui::DataCopyDialog)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
    mData = data;

    ui->comboType->addItem("C-Style BYTE (Hex)");
    ui->comboType->addItem("C-Style WORD (Hex)");
    ui->comboType->addItem("C-Style DWORD (Hex)");
    ui->comboType->addItem("C-Style QWORD (Hex)");
    ui->comboType->addItem("C-Style String");
    ui->comboType->addItem("C-Style Unicode String");
    ui->comboType->addItem("C-Style Shellcode String");
    ui->comboType->addItem("Pascal BYTE (Hex)");
    ui->comboType->addItem("Pascal WORD (Hex)");
    ui->comboType->addItem("Pascal DWORD (Hex)");
    ui->comboType->addItem("Pascal QWORD (Hex)");

    ui->comboType->setCurrentIndex(DataCByte);

    printData((DataType)ui->comboType->currentIndex());
}

QString DataCopyDialog::printEscapedString(bool & bPrevWasHex, int ch, const char* hexFormat)
{
    QString data = "";
    switch(ch) //escaping
    {
    case '\t':
        data = "\\t";
        bPrevWasHex = false;
        break;
    case '\f':
        data = "\\f";
        bPrevWasHex = false;
        break;
    case '\v':
        data = "\\v";
        bPrevWasHex = false;
        break;
    case '\n':
        data = "\\n";
        bPrevWasHex = false;
        break;
    case '\r':
        data = "\\r";
        bPrevWasHex = false;
        break;
    case '\\':
        data = "\\\\";
        bPrevWasHex = false;
        break;
    case '\"':
        data = "\\\"";
        bPrevWasHex = false;
        break;
    default:
        if(ch >= ' ' && ch <= '~')
        {
            if(bPrevWasHex && isxdigit(ch))
                data = QString().sprintf("\"\"%c", ch);
            else
                data = QString().sprintf("%c", ch);
            bPrevWasHex = false;
        }
        else
        {
            bPrevWasHex = true;
            data = QString().sprintf(hexFormat, ch);
        }
        break;
    }
    return data;
}

void DataCopyDialog::printData(DataType type)
{
    ui->editCode->clear();
    QString data;
    switch(type)
    {
    case DataCByte:
    {
        int numbytes = mData->size() / sizeof(unsigned char);
        data += "{";
        for(int i = 0; i < numbytes; i++)
        {
            if(i)
                data += ", ";
            data += QString().sprintf("0x%02X", ((unsigned char*)mData->constData())[i]);
        }
        data += "};";
    }
    break;

    case DataCWord:
    {
        int numwords = mData->size() / sizeof(unsigned short);
        data += "{";
        for(int i = 0; i < numwords; i++)
        {
            if(i)
                data += ", ";
            data += QString().sprintf("0x%04X", ((unsigned short*)mData->constData())[i]);
        }
        data += "};";
    }
    break;

    case DataCDword:
    {
        int numdwords = mData->size() / sizeof(unsigned int);
        data += "{";
        for(int i = 0; i < numdwords; i++)
        {
            if(i)
                data += ", ";
            data += QString().sprintf("0x%08X", ((unsigned int*)mData->constData())[i]);
        }
        data += "};";
    }
    break;

    case DataCQword:
    {
        int numqwords = mData->size() / sizeof(unsigned long long);
        data += "{";
        for(int i = 0; i < numqwords; i++)
        {
            if(i)
                data += ", ";
            data += QString().sprintf("0x%016llX", ((unsigned long long*)mData->constData())[i]);
        }
        data += "};";
    }
    break;

    case DataCString:
    {
        data += "\"";
        bool bPrevWasHex = false;
        for(int i = 0; i < mData->size(); i++)
        {
            byte_t ch = mData->at(i);
            data += printEscapedString(bPrevWasHex, ch, "\\x%02X");
        }
        data += "\"";
    }
    break;

    case DataCUnicodeString: //extended ASCII + hex escaped only
    {
        data += "L\"";
        int numwchars = mData->size() / sizeof(unsigned short);
        bool bPrevWasHex = false;
        for(int i = 0; i < numwchars; i++)
        {
            unsigned short ch = ((unsigned short*)mData->constData())[i];
            if((ch & 0xFF00) == 0) //extended ASCII
            {
                data += printEscapedString(bPrevWasHex, ch, "\\x%04X");
            }
            else //full unicode character
            {
                bPrevWasHex = true;
                data += QString().sprintf("\\x%04X", ch);
            }
        }
        data += "\"";
    }
    break;

    case DataCShellcodeString:
    {
        data += "\"";
        for(int i = 0; i < mData->size(); i++)
        {
            byte_t ch = mData->at(i);
            data += QString().sprintf("\\x%02X", ch);
        }
        data += "\"";
    }
    break;

    case DataPascalByte:
    {
        int numbytes = mData->size() / sizeof(unsigned char);
        data += QString().sprintf("Array [1..%u] of Byte = (", numbytes);
        for(int i = 0; i < numbytes; i++)
        {
            if(i)
                data += ", ";
            data += QString().sprintf("$%02X", ((unsigned char*)mData->constData())[i]);
        }
        data += ");";
    }
    break;

    case DataPascalWord:
    {
        int numwords = mData->size() / sizeof(unsigned short);
        data += QString().sprintf("Array [1..%u] of Word = (", numwords);
        for(int i = 0; i < numwords; i++)
        {
            if(i)
                data += ", ";
            data += QString().sprintf("$%04X", ((unsigned short*)mData->constData())[i]);
        }
        data += ");";
    }
    break;

    case DataPascalDword:
    {
        int numdwords = mData->size() / sizeof(unsigned int);
        data += QString().sprintf("Array [1..%u] of Dword = (", numdwords);
        for(int i = 0; i < numdwords; i++)
        {
            if(i)
                data += ", ";
            data += QString().sprintf("$%08X", ((unsigned int*)mData->constData())[i]);
        }
        data += ");";
    }
    break;

    case DataPascalQword:
    {
        int numqwords = mData->size() / sizeof(unsigned long long);
        data += QString().sprintf("Array [1..%u] of Int64 = (", numqwords);
        for(int i = 0; i < numqwords; i++)
        {
            if(i)
                data += ", ";
            data += QString().sprintf("$%016llX", ((unsigned long long*)mData->constData())[i]);
        }
        data += ");";
    }
    break;
    }
    ui->editCode->setPlainText(data);
}

DataCopyDialog::~DataCopyDialog()
{
    delete ui;
}

void DataCopyDialog::on_comboType_currentIndexChanged(int index)
{
    printData((DataType)index);
}

void DataCopyDialog::on_buttonCopy_clicked()
{
    Bridge::CopyToClipboard(ui->editCode->toPlainText());
}

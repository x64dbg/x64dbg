#include "DataCopyDialog.h"
#include "ui_DataCopyDialog.h"
#include "Bridge.h"

#define AF_INET6        23              // Internetwork Version 6
typedef PCTSTR(__stdcall* INETNTOPW)(INT Family, PVOID pAddr, wchar_t* pStringBuf, size_t StringBufSize);

DataCopyDialog::DataCopyDialog(const QVector<byte_t>* data, QWidget* parent) : QDialog(parent), ui(new Ui::DataCopyDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    mData = data;

    ui->comboType->addItem(tr("C-Style BYTE (Hex)"));
    ui->comboType->addItem(tr("C-Style WORD (Hex)"));
    ui->comboType->addItem(tr("C-Style DWORD (Hex)"));
    ui->comboType->addItem(tr("C-Style QWORD (Hex)"));
    ui->comboType->addItem(tr("C-Style String"));
    ui->comboType->addItem(tr("C-Style Unicode String"));
    ui->comboType->addItem(tr("C-Style Shellcode String"));
    ui->comboType->addItem(tr("Pascal BYTE (Hex)"));
    ui->comboType->addItem(tr("Pascal WORD (Hex)"));
    ui->comboType->addItem(tr("Pascal DWORD (Hex)"));
    ui->comboType->addItem(tr("Pascal QWORD (Hex)"));
    ui->comboType->addItem(tr("GUID"));
    ui->comboType->addItem(tr("IP Address (IPv4)"));
    ui->comboType->addItem(tr("IP Address (IPv6)"));
    ui->comboType->addItem(tr("Base64"));

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

    case DataGUID:
    {
        int numguids = mData->size() / sizeof(GUID);
        for(int i = 0; i < numguids; i++)
        {
            if(i)
                data += ", ";
            GUID guid = ((GUID*)(mData->constData()))[i];
            data += QString().sprintf("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        }
    }
    break;

    case DataIPv4:
    {
        int numIPs = mData->size() / 4;
        for(int i = 0; i < numIPs; i++)
        {
            if(i)
                data += ", ";
            data += QString("%1.%2.%3.%4").arg(mData->constData()[i * 4]).arg(mData->constData()[i * 4 + 1]).arg(mData->constData()[i * 4 + 2]).arg(mData->constData()[i * 4 + 3]);
        }
    }
    break;

    case DataIPv6:
    {
        INETNTOPW InetNtopW;
        int numIPs = mData->size() / 16;
        HMODULE hWinsock = LoadLibrary(L"ws2_32.dll");
        InetNtopW = INETNTOPW(GetProcAddress(hWinsock, "InetNtopW"));
        if(InetNtopW)
        {
            for(int i = 0; i < numIPs; i++)
            {
                if(i)
                    data += ", ";
                wchar_t buffer[56];
                memset(buffer, 0, sizeof(buffer));
                InetNtopW(AF_INET6, const_cast<byte_t*>(mData->constData() + i * 16), buffer, 56);
                data += QString::fromWCharArray(buffer);
            }
        }
        else //fallback for Windows XP
        {
            for(int i = 0; i < numIPs; i++)
            {
                if(i)
                    data += ", ";
                QString temp(QByteArray(reinterpret_cast<const char*>(mData->constData() + i * 16), 16).toHex());
                temp.insert(28, ':');
                temp.insert(24, ':');
                temp.insert(20, ':');
                temp.insert(16, ':');
                temp.insert(12, ':');
                temp.insert(8, ':');
                temp.insert(4, ':');
                data += temp;
            }
        }
        FreeLibrary(hWinsock);
    }
    break;

    case DataBase64:
    {
        data = QByteArray(reinterpret_cast<const char*>(mData->constData()), mData->size()).toBase64().constData();
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

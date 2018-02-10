#include "DataCopyDialog.h"
#include "ui_DataCopyDialog.h"
#include "Bridge.h"
#include <QCryptographicHash>
#include <QTextCodec>
#include <QMessageBox>

#define AF_INET6        23              // Internetwork Version 6
typedef PCTSTR(__stdcall* INETNTOPW)(INT Family, PVOID pAddr, wchar_t* pStringBuf, size_t StringBufSize);

DataCopyDialog::DataCopyDialog(const QVector<byte_t>* data, QWidget* parent) : QDialog(parent), ui(new Ui::DataCopyDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    mData = data;

    mTypes[DataCByte] = FormatType { tr("C-Style BYTE (Hex)"), 16 };
    mTypes[DataCWord] = FormatType { tr("C-Style WORD (Hex)"), 12 };
    mTypes[DataCDword] = FormatType { tr("C-Style DWORD (Hex)"), 8 };
    mTypes[DataCQword] = FormatType { tr("C-Style QWORD (Hex)"), 4 };
    mTypes[DataCString] = FormatType { tr("C-Style String"), 1 };
    mTypes[DataCUnicodeString] = FormatType { tr("C-Style Unicode String"), 1 };
    mTypes[DataCShellcodeString] = FormatType { tr("C-Style Shellcode String"), 1 };
    mTypes[DataASMByte] = FormatType { tr("ASM-Style BYTE (Hex)"), 16 };
    mTypes[DataASMWord] = FormatType { tr("ASM-Style WORD (Hex)"), 12 };
    mTypes[DataASMDWord] = FormatType { tr("ASM-Style DWORD (Hex)"), 8 };
    mTypes[DataASMQWord] = FormatType { tr("ASM-Style QWORD (Hex)"), 4 };
    mTypes[DataASMString] = FormatType { tr("ASM-Style String"), 4 };
    mTypes[DataPascalByte] = FormatType { tr("Pascal BYTE (Hex)"), 42 };
    mTypes[DataPascalWord] = FormatType { tr("Pascal WORD (Hex)"), 21 };
    mTypes[DataPascalDword] = FormatType { tr("Pascal DWORD (Hex)"), 10 };
    mTypes[DataPascalQword] = FormatType { tr("Pascal QWORD (Hex)"), 5 };
    mTypes[DataString] = FormatType { tr("String"), 1 };
    mTypes[DataUnicodeString] = FormatType { tr("Unicode String"), 1 };
    mTypes[DataUTF8String] = FormatType { tr("UTF8 String"), 1 };
    mTypes[DataUCS4String] = FormatType { tr("UCS4 String"), 1 };
    mTypes[DataHexStream] = FormatType { tr("Hex Stream"), 1 };
    mTypes[DataGUID] = FormatType { tr("GUID"), 1 };
    mTypes[DataIPv4] = FormatType { tr("IP Address (IPv4)"), 5 };
    mTypes[DataIPv6] = FormatType { tr("IP Address (IPv6)"), 1 };
    mTypes[DataBase64] = FormatType { tr("Base64"), 1 };
    mTypes[DataMD5] = FormatType { "MD5", 1};
    mTypes[DataSHA1] = FormatType { "SHA1", 1};
    mTypes[DataSHA256] = FormatType { "SHA256 (SHA-2)", 1};
    mTypes[DataSHA512] = FormatType { "SHA512 (SHA-2)", 1};
    mTypes[DataSHA256_3] = FormatType { "SHA256 (SHA-3)", 1};
    mTypes[DataSHA512_3] = FormatType { "SHA512 (SHA-3)", 1};

    for(int i = 0; i < DataLast; i++)
        ui->listType->addItem(mTypes[i].name);

    QModelIndex index = ui->listType->model()->index(DataCByte, 0);
    ui->listType->setCurrentIndex(index);

    printData((DataType)ui->listType->currentIndex().row());
    Config()->setupWindowPos(this);
}

static QString printEscapedString(bool & bPrevWasHex, int ch, const char* hexFormat)
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
    case '\a':
        data = "\\a";
        bPrevWasHex = false;
        break;
    case '\b':
        data = "\\b";
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

template<typename T>
static QString formatLoop(const QVector<byte_t>* bytes, int itemsPerLine, QString(*format)(T))
{
    QString data;
    int count = bytes->size() / sizeof(T);
    for(int i = 0; i < count; i++)
    {
        if(i)
        {
            data += ',';
            if(itemsPerLine > 0 && i % itemsPerLine == 0)
                data += '\n';
            else
                data += ' ';
        }

        data += format(((const T*)bytes->constData())[i]);
    }
    return data;
}

static QString printHash(const QVector<byte_t>* bytes, QCryptographicHash::Algorithm algorithm)
{
    QCryptographicHash temp(algorithm);
    temp.addData((const char*)bytes->data(), bytes->size());
    return temp.result().toHex().toUpper();
}

void DataCopyDialog::printData(DataType type)
{
    ui->editCode->clear();
    QString data;
    switch(type)
    {
    case DataCByte:
    {
        data = "{\n" + formatLoop<unsigned char>(mData, mTypes[mIndex].itemsPerLine, [](unsigned char n)
        {
            return QString().sprintf("0x%02X", n);
        }) + "\n};";
    }
    break;

    case DataCWord:
    {
        data = "{\n" + formatLoop<unsigned short>(mData, mTypes[mIndex].itemsPerLine, [](unsigned short n)
        {
            return QString().sprintf("0x%04X", n);
        }) + "\n};";
    }
    break;

    case DataCDword:
    {
        data = "{\n" + formatLoop<unsigned int>(mData, mTypes[mIndex].itemsPerLine, [](unsigned int n)
        {
            return QString().sprintf("0x%08X", n);
        }) + "\n};";
    }
    break;

    case DataCQword:
    {
        data = "{\n" + formatLoop<unsigned long long>(mData, mTypes[mIndex].itemsPerLine, [](unsigned long long n)
        {
            return QString().sprintf("0x%016llX", n);
        }) + "\n};";
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

    case DataString:
    {
        data = QTextCodec::codecForName("System")->makeDecoder()->toUnicode((const char*)(mData->data()), mData->size());
    }
    break;

    case DataUnicodeString:
    {
        data = QString::fromUtf16((const ushort*)(mData->data()), mData->size() / 2);
    }
    break;

    case DataUTF8String:
    {
        data = QString::fromUtf8((const char*)mData->data(), mData->size());
    }
    break;

    case DataUCS4String:
    {
        data = QString::fromUcs4((const uint*)(mData->data()), mData->size() / 4);
    }
    break;

    case DataASMByte:
    {
        data = "array DB " + formatLoop<unsigned char>(mData, mTypes[mIndex].itemsPerLine, [](unsigned char n)
        {
            QString value = QString().sprintf("%02Xh", n);
            if(value.at(0).isLetter())
                value.insert(0, '0');

            return value;
        });
    }
    break;

    case DataASMWord:
    {
        data = "array DW " + formatLoop<unsigned short>(mData, mTypes[mIndex].itemsPerLine, [](unsigned short n)
        {
            QString value = QString().sprintf("%04Xh", n);
            if(value.at(0).isLetter())
                value.insert(0, '0');

            return value;
        });
    }
    break;

    case DataASMDWord:
    {
        data = "array DD " + formatLoop<unsigned int>(mData, mTypes[mIndex].itemsPerLine, [](unsigned int n)
        {
            QString value = QString().sprintf("%08Xh", n);
            if(value.at(0).isLetter())
                value.insert(0, '0');

            return value;
        });
    }
    break;

    case DataASMQWord:
    {
        data = "array DQ " + formatLoop<unsigned long long>(mData, mTypes[mIndex].itemsPerLine, [](unsigned long long n)
        {
            QString value = QString().sprintf("%016llXh", n);
            if(value.at(0).isLetter())
                value.insert(0, '0');

            return value;
        });
    }
    break;

    case DataASMString:
    {
        QString line;
        int index = 0;
        bool bPrevWasHex = false;
        while(index < mData->size())
        {
            QChar chr = QChar(mData->at(index));
            if(chr >= ' ' && chr <= '~')
            {
                if(line.length() == 0)
                    line += "\"";

                if(bPrevWasHex)
                {
                    line += ",\"";
                    bPrevWasHex = false;
                }

                line += chr;
            }
            else
            {
                QString asmhex = QString().sprintf("%02Xh", mData->at(index));
                if(asmhex.at(0).isLetter())
                    asmhex.insert(0, "0");

                if(line.length() == 0)
                    line += asmhex;
                else if(!bPrevWasHex)
                    line += "\"," + asmhex;
                else
                    line += "," + asmhex;

                bPrevWasHex = true;
            }

            index++;
        }

        if(!bPrevWasHex)
            line += "\"";

        data = line;
    }
    break;

    case DataPascalByte:
    {
        data += QString().sprintf("Array [1..%u] of Byte = (\n", mData->size());
        data += formatLoop<unsigned char>(mData, mTypes[mIndex].itemsPerLine, [](unsigned char n)
        {
            return QString().sprintf("$%02X", n);
        });
        data += "\n);";
    }
    break;

    case DataPascalWord:
    {
        data += QString().sprintf("Array [1..%u] of Word = (\n", mData->size() / 2);
        data += formatLoop<unsigned short>(mData, mTypes[mIndex].itemsPerLine, [](unsigned short n)
        {
            return QString().sprintf("$%04X", n);
        });
        data += "\n);";
    }
    break;

    case DataPascalDword:
    {
        data += QString().sprintf("Array [1..%u] of Dword = (\n", mData->size() / 4);
        data += formatLoop<unsigned int>(mData, mTypes[mIndex].itemsPerLine, [](unsigned int n)
        {
            return QString().sprintf("$%08X", n);
        });
        data += "\n);";
    }
    break;

    case DataPascalQword:
    {
        data += QString().sprintf("Array [1..%u] of Int64 = (\n", mData->size() / 8);
        data += formatLoop<unsigned long long>(mData, mTypes[mIndex].itemsPerLine, [](unsigned long long n)
        {
            return QString().sprintf("$%016llX", n);
        });
        data += "\n);";
    }
    break;

    case DataHexStream:
    {
        for(int i = 0; i < mData->size(); i++)
            data += QString().sprintf("%02X", mData->constData()[i]);
    }
    break;

    case DataGUID:
    {
        data = formatLoop<GUID>(mData, mTypes[mIndex].itemsPerLine, [](GUID guid)
        {
            return QString().sprintf("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        });
    }
    break;

    case DataIPv4:
    {
        int numIPs = mData->size() / 4;
        for(int i = 0; i < numIPs; i++)
        {
            if(i)
            {
                if((i % mTypes[mIndex].itemsPerLine) == 0)
                    data += "\n";
                else
                    data += ", ";
            }
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
                {
                    if((i % mTypes[mIndex].itemsPerLine) == 0)
                        data += "\n";
                    else
                        data += ", ";
                }
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
                {
                    if((i % mTypes[mIndex].itemsPerLine) == 0)
                        data += "\n";
                    else
                        data += ", ";
                }
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

    case DataMD5:
        data = printHash(mData, QCryptographicHash::Md5);
        break;

    case DataSHA1:
        data = printHash(mData, QCryptographicHash::Sha1);
        break;

    case DataSHA256:
        data = printHash(mData, QCryptographicHash::Sha256);
        break;

    case DataSHA512:
        data = printHash(mData, QCryptographicHash::Sha512);
        break;

    case DataSHA256_3:
        data = printHash(mData, QCryptographicHash::Sha3_256);
        break;

    case DataSHA512_3:
        data = printHash(mData, QCryptographicHash::Sha3_512);
        break;
    }
    ui->editCode->setPlainText(data);
}

DataCopyDialog::~DataCopyDialog()
{
    Config()->saveWindowPos(this);
    delete ui;
}

void DataCopyDialog::on_listType_currentRowChanged(int currentRow)
{
    mIndex = currentRow;
    ui->spinBox->setValue(mTypes[mIndex].itemsPerLine);
    printData(DataType(mIndex));
}

void DataCopyDialog::on_buttonCopy_clicked()
{
    Bridge::CopyToClipboard(ui->editCode->toPlainText());
}

void DataCopyDialog::on_spinBox_valueChanged(int value)
{
    mTypes[mIndex].itemsPerLine = value;
    printData(DataType(mIndex));
}

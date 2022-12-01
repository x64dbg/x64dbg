#include <QTextCodec>
#include <QCryptographicHash>
#include "Configuration.h"
#include "HexEditDialog.h"
#include "QHexEdit/QHexEdit.h"
#include "ui_HexEditDialog.h"
#include "Configuration.h"
#include "Bridge.h"
#include "CodepageSelectionDialog.h"
#include "LineEditDialog.h"

#ifndef AF_INET6
#define AF_INET6        23              // Internetwork Version 6
#endif //AF_INET6
typedef PCTSTR(__stdcall* INETNTOPW)(INT Family, PVOID pAddr, wchar_t* pStringBuf, size_t StringBufSize);

HexEditDialog::HexEditDialog(QWidget* parent) : QDialog(parent), ui(new Ui::HexEditDialog)
{
    ui->setupUi(this);
    ui->editCode->setFont(ConfigFont("HexEdit"));

    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) | Qt::WindowMaximizeButtonHint);
    setModal(true); //modal window

    //setup text fields
    ui->lineEditAscii->setEncoding(QTextCodec::codecForName("System"));
    ui->lineEditUnicode->setEncoding(QTextCodec::codecForName("UTF-16"));
    ui->chkKeepSize->setChecked(ConfigBool("HexDump", "KeepSize"));
    ui->chkEntireBlock->hide();

    mDataInitialized = false;
    stringEditorLock = false;
    fallbackCodec = QTextCodec::codecForLocale();

    //setup hex editor
    mHexEdit = new QHexEdit(this);
    mHexEdit->setEditFont(ConfigFont("HexEdit"));
    mHexEdit->setHorizontalSpacing(6);
    mHexEdit->setOverwriteMode(true);
    ui->scrollArea->setWidget(mHexEdit);
    mHexEdit->widget()->setFocus();
    connect(mHexEdit, SIGNAL(dataChanged()), this, SLOT(dataChangedSlot()));
    connect(mHexEdit, SIGNAL(dataEdited()), this, SLOT(dataEditedSlot()));
    connect(ui->btnCodePage2, SIGNAL(clicked()), this, SLOT(on_btnCodepage_clicked()));
    connect(ui->chkCRLF, SIGNAL(clicked()), this, SLOT(on_stringEditor_textChanged()));

    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateStyle()));

    updateStyle();
    updateCodepage();

    //The following initialization code is from Data Copy Dialog.
    mTypes[DataCByte] = FormatType { tr("C-Style BYTE (Hex)"), 16 };
    mTypes[DataCWord] = FormatType { tr("C-Style WORD (Hex)"), 12 };
    mTypes[DataCDword] = FormatType { tr("C-Style DWORD (Hex)"), 8 };
    mTypes[DataCQword] = FormatType { tr("C-Style QWORD (Hex)"), 4 };
    mTypes[DataCString] = FormatType { tr("C-Style String"), 1 };
    mTypes[DataCUnicodeString] = FormatType { tr("C-Style Unicode String"), 1 };
    mTypes[DataCShellcodeString] = FormatType { tr("C-Style Shellcode String"), 1 };
    mTypes[DataASMByte] = FormatType { tr("ASM-Style BYTE (Hex)"), 16, "DB"};
    mTypes[DataASMWord] = FormatType { tr("ASM-Style WORD (Hex)"), 12, "DW"};
    mTypes[DataASMDWord] = FormatType { tr("ASM-Style DWORD (Hex)"), 8, "DD"};
    mTypes[DataASMQWord] = FormatType { tr("ASM-Style QWORD (Hex)"), 4, "DQ"};
    mTypes[DataASMString] = FormatType { tr("ASM-Style String"), 4 };
    mTypes[DataPascalByte] = FormatType { tr("Pascal BYTE (Hex)"), 42 };
    mTypes[DataPascalWord] = FormatType { tr("Pascal WORD (Hex)"), 21 };
    mTypes[DataPascalDword] = FormatType { tr("Pascal DWORD (Hex)"), 10 };
    mTypes[DataPascalQword] = FormatType { tr("Pascal QWORD (Hex)"), 5 };
    mTypes[DataPython3Byte] = FormatType { tr("Python 3 BYTE (Hex)"), 1 };
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

    duint lastDataType = ConfigUint("HexDump", "CopyDataType");
    lastDataType = std::min(lastDataType, static_cast<duint>(ui->listType->count() - 1));
    QModelIndex index = ui->listType->model()->index(lastDataType, 0);
    ui->listType->setCurrentIndex(index);

    Config()->setupWindowPos(this);
}

HexEditDialog::~HexEditDialog()
{
    Config()->saveWindowPos(this);
    delete ui;
}

void HexEditDialog::showEntireBlock(bool show, bool checked)
{
    ui->chkEntireBlock->setVisible(show);
    ui->chkEntireBlock->setChecked(checked);
}

void HexEditDialog::showKeepSize(bool show)
{
    ui->chkKeepSize->setVisible(show);
}

void HexEditDialog::isDataCopiable(bool copyDataEnabled)
{
    if(copyDataEnabled == false)
        ui->tabModeSelect->removeTab(2); //This can't be undone!
}

void HexEditDialog::updateCodepage()
{
    duint lastCodepage;
    auto allCodecs = QTextCodec::availableCodecs();
    if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
        lastCodec = fallbackCodec;
    else
        lastCodec = QTextCodec::codecForName(allCodecs.at(lastCodepage));
    ui->lineEditCodepage->setEncoding(lastCodec);
    ui->lineEditCodepage->setData(mHexEdit->data());
    ui->stringEditor->document()->setPlainText(lastCodec->toUnicode(mHexEdit->data()));
    ui->labelLastCodepage->setText(lastCodec->name().constData());
    ui->labelLastCodepage2->setText(ui->labelLastCodepage->text());
}

void HexEditDialog::updateCodepage(const QByteArray & name)
{
    lastCodec = QTextCodec::codecForName(name);
    if(!lastCodec)
        lastCodec = fallbackCodec;
    ui->lineEditCodepage->setEncoding(lastCodec);
    ui->lineEditCodepage->setData(mHexEdit->data());
    ui->stringEditor->document()->setPlainText(lastCodec->toUnicode(mHexEdit->data()));
    ui->labelLastCodepage->setText(QString(name));
    ui->labelLastCodepage2->setText(ui->labelLastCodepage->text());
}

bool HexEditDialog::entireBlock()
{
    return ui->chkEntireBlock->isChecked();
}

void HexEditDialog::updateStyle()
{
    QString style = QString("QLineEdit { border-style: outset; border-width: 1px; border-color: %1; color: %1; background-color: %2 }").arg(ConfigColor("HexEditTextColor").name(), ConfigColor("HexEditBackgroundColor").name());
    ui->lineEditAscii->setStyleSheet(style);
    ui->lineEditUnicode->setStyleSheet(style);
    ui->lineEditCodepage->setStyleSheet(style);

    mHexEdit->setTextColor(ConfigColor("HexEditTextColor"));
    mHexEdit->setWildcardColor(ConfigColor("HexEditWildcardColor"));
    mHexEdit->setBackgroundColor(ConfigColor("HexEditBackgroundColor"));
    mHexEdit->setSelectionColor(ConfigColor("HexEditSelectionColor"));
}

void HexEditDialog::on_chkKeepSize_toggled(bool checked)
{
    if(!this->isVisible())
        return;
    mHexEdit->setKeepSize(checked);
    ui->lineEditAscii->setKeepSize(checked);
    ui->lineEditUnicode->setKeepSize(checked);
    ui->lineEditCodepage->setKeepSize(checked);
    Config()->setBool("HexDump", "KeepSize", checked);
}

void HexEditDialog::dataChangedSlot()
{
    // Allows initialization of the data by calling setData() on mHexEdit.
    if(!mDataInitialized)
    {
        QByteArray data = mHexEdit->data();
        ui->lineEditAscii->setData(data);
        ui->lineEditUnicode->setData(data);
        ui->lineEditCodepage->setData(data);
        ui->stringEditor->document()->setPlainText(lastCodec->toUnicode(data));
        checkDataRepresentable(0);

        printData((DataType)ui->listType->currentIndex().row());

        mDataInitialized = true;
    }
}

void HexEditDialog::dataEditedSlot()
{
    QByteArray data = mHexEdit->data();
    ui->lineEditAscii->setData(data);
    ui->lineEditUnicode->setData(data);
    ui->lineEditCodepage->setData(data);
    ui->stringEditor->document()->setPlainText(lastCodec->toUnicode(data));
    printData((DataType)ui->listType->currentIndex().row());
    checkDataRepresentable(0);
}

void HexEditDialog::on_lineEditAscii_dataEdited()
{
    QByteArray data = ui->lineEditAscii->data();
    data = resizeData(data);
    ui->lineEditUnicode->setData(data);
    ui->lineEditCodepage->setData(data);
    ui->stringEditor->document()->setPlainText(lastCodec->toUnicode(data));
    printData((DataType)ui->listType->currentIndex().row());
    mHexEdit->setData(data);
}

void HexEditDialog::on_lineEditUnicode_dataEdited()
{
    QByteArray data = ui->lineEditUnicode->data();
    data = resizeData(data);
    ui->lineEditAscii->setData(data);
    ui->lineEditCodepage->setData(data);
    ui->stringEditor->document()->setPlainText(lastCodec->toUnicode(data));
    printData((DataType)ui->listType->currentIndex().row());
    mHexEdit->setData(data);
}

void HexEditDialog::on_stringEditor_textChanged() //stack overflow?
{
    if(stringEditorLock || ui->tabModeSelect->currentIndex() != 1) return;
    stringEditorLock = true;
    QTextCodec::ConverterState converter(QTextCodec::IgnoreHeader); //Don't add BOM for UTF-16
    QString text = ui->stringEditor->document()->toPlainText();
    if(ui->chkCRLF->checkState() == Qt::Checked)
    {
        text = text.replace(QChar('\n'), "\r\n");
        text = text.replace("\r\r\n", "\r\n");
    }
    QByteArray data = lastCodec->fromUnicode(text.constData(), text.size(), &converter);
    data = resizeData(data);
    ui->lineEditAscii->setData(data);
    ui->lineEditUnicode->setData(data);
    ui->lineEditCodepage->setData(data);
    printData((DataType)ui->listType->currentIndex().row());
    mHexEdit->setData(data);
    stringEditorLock = false;
}

void HexEditDialog::on_lineEditCodepage_dataEdited()
{
    QByteArray data = ui->lineEditCodepage->data();
    data = resizeData(data);
    ui->lineEditAscii->setData(data);
    ui->lineEditUnicode->setData(data);
    ui->stringEditor->document()->setPlainText(lastCodec->toUnicode(data));
    mHexEdit->setData(data);
}

QByteArray HexEditDialog::resizeData(QByteArray & data)
{
    // truncate or pad the data
    if(mHexEdit->keepSize())
    {
        int dataSize = mHexEdit->data().size();
        int data_size = data.size();
        if(dataSize < data_size)
            data.resize(dataSize);
        else if(dataSize > data_size)
            data.append(QByteArray(dataSize - data_size, 0));
    }

    return data;
}

void HexEditDialog::on_btnCodepage_clicked()
{
    CodepageSelectionDialog codepageDialog(this);
    if(codepageDialog.exec() != QDialog::Accepted)
        return;
    stringEditorLock = true;
    updateCodepage(codepageDialog.getSelectedCodepage());
    checkDataRepresentable(3);
    checkDataRepresentable(4);
    stringEditorLock = false;
}

bool HexEditDialog::checkDataRepresentable(int mode)
{
    QTextCodec* codec;
    QLabel* label;
    if(mode == 1)
    {
        codec = QTextCodec::codecForName("System");
        label = ui->labelWarningCodepageASCII;
    }
    else if(mode == 2)
    {
        codec = QTextCodec::codecForName("UTF-16");
        label = ui->labelWarningCodepageUTF;
    }
    else if(mode == 3)
    {
        codec = lastCodec;
        label = ui->labelWarningCodepage;
    }
    else if(mode == 4)
    {
        codec = lastCodec;
        label = ui->labelWarningCodepageString;
    }
    else
    {
        bool isRepresentable;
        isRepresentable = checkDataRepresentable(1);
        isRepresentable = checkDataRepresentable(2) && isRepresentable;
        isRepresentable = checkDataRepresentable(3) && isRepresentable;
        isRepresentable = checkDataRepresentable(4) && isRepresentable;
        return isRepresentable;
    }
    if(codec == nullptr)
    {
        label->show();
        return false;
    }
    QString test;
    QByteArray original = mHexEdit->data();
    QTextCodec::ConverterState converter(QTextCodec::IgnoreHeader);
    test = codec->toUnicode(original);
    QByteArray test2;
    test2 = codec->fromUnicode(test.constData(), test.size(), &converter);
    if(test2.size() < original.size() || memcmp(test2.constData(), original.constData(), original.size()) != 0)
    {
        label->show();
        return false;
    }
    else
    {
        label->hide();
        return true;
    }
}

//The following code is from Data Copy Dialog.
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
static QString formatLoop(const QByteArray & bytes, int itemsPerLine, QString linePrefix, QString(*format)(T))
{
    QString data;
    int count = bytes.size() / sizeof(T);
    for(int i = 0; i < count; i++)
    {
        if(i)
        {
            data += ',';
            if(itemsPerLine > 0 && i % itemsPerLine == 0)
                data += (QString('\n') + linePrefix + " ");
            else
                data += ' ';
        }

        data += format(((const T*)bytes.constData())[i]);
    }
    return data;
}

static QString printHash(const QByteArray & bytes, QCryptographicHash::Algorithm algorithm)
{
    QCryptographicHash temp(algorithm);
    temp.addData((const char*)bytes.constData(), bytes.size());
    return temp.result().toHex().toUpper();
}


void HexEditDialog::printData(DataType type)
{
    ui->editCode->clear();
    QString data;
    QByteArray mData = mHexEdit->data();
    switch(type)
    {
    case DataCByte:
    {
        data = "{\n" + formatLoop<unsigned char>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned char n)
        {
            return QString().sprintf("0x%02X", n);
        }) + "\n};";
    }
    break;

    case DataCWord:
    {
        data = "{\n" + formatLoop<unsigned short>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned short n)
        {
            return QString().sprintf("0x%04X", n);
        }) + "\n};";
    }
    break;

    case DataCDword:
    {
        data = "{\n" + formatLoop<unsigned int>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned int n)
        {
            return QString().sprintf("0x%08X", n);
        }) + "\n};";
    }
    break;

    case DataCQword:
    {
        data = "{\n" + formatLoop<unsigned long long>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned long long n)
        {
            return QString().sprintf("0x%016llX", n);
        }) + "\n};";
    }
    break;

    case DataCString:
    {
        data += "\"";
        bool bPrevWasHex = false;
        for(int i = 0; i < mData.size(); i++)
        {
            byte_t ch = mData.at(i);
            data += printEscapedString(bPrevWasHex, ch, "\\x%02X");
        }
        data += "\"";
    }
    break;

    case DataCUnicodeString: //extended ASCII + hex escaped only
    {
        data += "L\"";
        int numwchars = mData.size() / sizeof(unsigned short);
        bool bPrevWasHex = false;
        for(int i = 0; i < numwchars; i++)
        {
            unsigned short ch = ((unsigned short*)mData.constData())[i];
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
        for(int i = 0; i < mData.size(); i++)
        {
            byte_t ch = mData.at(i);
            data += QString().sprintf("\\x%02X", ch);
        }
        data += "\"";
    }
    break;

    case DataString:
    {
        data = QTextCodec::codecForName("System")->makeDecoder()->toUnicode((const char*)(mData.constData()), mData.size());
    }
    break;

    case DataUnicodeString:
    {
        data = QString::fromUtf16((const ushort*)(mData.constData()), mData.size() / 2);
    }
    break;

    case DataUTF8String:
    {
        data = QString::fromUtf8((const char*)mData.constData(), mData.size());
    }
    break;

    case DataUCS4String:
    {
        data = QString::fromUcs4((const uint*)(mData.constData()), mData.size() / 4);
    }
    break;

    case DataASMByte:
    {
        data = "array DB " + formatLoop<unsigned char>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned char n)
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
        data = "array DW " + formatLoop<unsigned short>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned short n)
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
        data = "array DD " + formatLoop<unsigned int>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned int n)
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
        data = "array DQ " + formatLoop<unsigned long long>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned long long n)
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
        while(index < mData.size())
        {
            QChar chr = QChar(mData.at(index));
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
                QString asmhex = QString().sprintf("%02Xh", mData.at(index));
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
        data += QString().sprintf("Array [1..%u] of Byte = (\n", mData.size());
        data += formatLoop<unsigned char>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned char n)
        {
            return QString().sprintf("$%02X", n);
        });
        data += "\n);";
    }
    break;

    case DataPascalWord:
    {
        data += QString().sprintf("Array [1..%u] of Word = (\n", mData.size() / 2);
        data += formatLoop<unsigned short>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned short n)
        {
            return QString().sprintf("$%04X", n);
        });
        data += "\n);";
    }
    break;

    case DataPascalDword:
    {
        data += QString().sprintf("Array [1..%u] of Dword = (\n", mData.size() / 4);
        data += formatLoop<unsigned int>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned int n)
        {
            return QString().sprintf("$%08X", n);
        });
        data += "\n);";
    }
    break;

    case DataPascalQword:
    {
        data += QString().sprintf("Array [1..%u] of Int64 = (\n", mData.size() / 8);
        data += formatLoop<unsigned long long>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](unsigned long long n)
        {
            return QString().sprintf("$%016llX", n);
        });
        data += "\n);";
    }
    break;

    case DataPython3Byte:
    {
        data += "b\"";
        for(int i = 0; i < mData.size(); i++)
        {
            byte_t ch = mData.at(i);
            data += QString().sprintf("\\x%02X", ch);
        }
        data += "\"";
    }
    break;

    case DataHexStream:
    {
        for(int i = 0; i < mData.size(); i++)
        {
            byte_t ch = mData.at(i);
            data += QString().sprintf("%02X", ch);
        }
    }
    break;

    case DataGUID:
    {
        data = formatLoop<GUID>(mData, mTypes[mIndex].itemsPerLine, mTypes[mIndex].linePrefix, [](GUID guid)
        {
            return QString().sprintf("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        });
    }
    break;

    case DataIPv4:
    {
        int numIPs = mData.size() / 4;
        for(int i = 0; i < numIPs; i++)
        {
            if(i)
            {
                if((i % mTypes[mIndex].itemsPerLine) == 0)
                    data += "\n";
                else
                    data += ", ";
            }
            data += QString("%1.%2.%3.%4").arg((unsigned char)(mData.constData()[i * 4])).arg((unsigned char)(mData.constData()[i * 4 + 1]))
                    .arg((unsigned char)(mData.constData()[i * 4 + 2])).arg((unsigned char)(mData.constData()[i * 4 + 3]));
        }
    }
    break;

    case DataIPv6:
    {
        INETNTOPW InetNtopW;
        int numIPs = mData.size() / 16;
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
                InetNtopW(AF_INET6, const_cast<char*>(mData.constData() + i * 16), buffer, 56);
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
                QString temp(QByteArray(reinterpret_cast<const char*>(mData.constData() + i * 16), 16).toHex());
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
        data = QByteArray(reinterpret_cast<const char*>(mData.constData()), mData.size()).toBase64().constData();
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


void HexEditDialog::on_listType_currentRowChanged(int currentRow)
{
    mIndex = currentRow;
    ui->spinBox->setValue(mTypes[mIndex].itemsPerLine);
    printData(DataType(mIndex));
    Config()->setUint("HexDump", "CopyDataType", currentRow);
}

void HexEditDialog::on_buttonCopy_clicked()
{
    Bridge::CopyToClipboard(ui->editCode->toPlainText());
}

void HexEditDialog::on_spinBox_valueChanged(int value)
{
    mTypes[mIndex].itemsPerLine = value;
    printData(DataType(mIndex));
}

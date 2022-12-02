#pragma once

#include <QDialog>
#include "QHexEdit/QHexEdit.h"

namespace Ui
{
    class HexEditDialog;
}

class HexEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HexEditDialog(QWidget* parent = 0);
    ~HexEditDialog();

    void showEntireBlock(bool show, bool checked = false);
    void showKeepSize(bool show);
    void isDataCopiable(bool copyDataEnabled);
    void updateCodepage();

    bool entireBlock();

    QHexEdit* mHexEdit;

    struct FormatType
    {
        QString name;
        int itemsPerLine;
        QString linePrefix;
    };

private slots:
    void updateStyle();
    void on_chkKeepSize_toggled(bool checked);
    void dataChangedSlot();
    void dataEditedSlot();
    void on_lineEditAscii_dataEdited();
    void on_lineEditUnicode_dataEdited();
    void on_lineEditCodepage_dataEdited();
    void on_btnCodepage_clicked();
    void on_stringEditor_textChanged();

private:
    Ui::HexEditDialog* ui;
    void updateCodepage(const QByteArray & name);
    QTextCodec* lastCodec;
    QTextCodec* fallbackCodec;
    bool stringEditorLock;

    bool mDataInitialized;

    QByteArray resizeData(QByteArray & data);
    bool checkDataRepresentable(int mode); //1=ASCII, 2=Unicode, 3=User-selected codepage, 4=String editor, others(0)=All modes

    //The following code is from Data Copy Dialog
private slots:
    void on_listType_currentRowChanged(int currentRow);
    void on_buttonCopy_clicked();
    void on_spinBox_valueChanged(int arg1);

private:
    int mIndex;

    enum DataType
    {
        DataCByte = 0,
        DataCWord,
        DataCDword,
        DataCQword,
        DataCString,
        DataCUnicodeString,
        DataCShellcodeString,
        DataASMByte,
        DataASMWord,
        DataASMDWord,
        DataASMQWord,
        DataASMString,
        DataPascalByte,
        DataPascalWord,
        DataPascalDword,
        DataPascalQword,
        DataPython3Byte,
        DataString,
        DataUnicodeString,
        DataUTF8String,
        DataUCS4String,
        DataHexStream,
        DataGUID,
        DataIPv4,
        DataIPv6,
        DataBase64,
        DataMD5,
        DataSHA1,
        DataSHA256,
        DataSHA512,
        DataSHA256_3,
        DataSHA512_3,
        DataLast
    };

    FormatType mTypes[DataLast];

    void printData(DataType type);
};

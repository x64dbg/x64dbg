#ifndef DATACOPYDIALOG_H
#define DATACOPYDIALOG_H

#include <QDialog>
#include <QVector>
#include "Imports.h"

namespace Ui
{
    class DataCopyDialog;
}

class DataCopyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataCopyDialog(const QVector<byte_t>* data, QWidget* parent = 0);
    ~DataCopyDialog();

private slots:
    void on_listType_currentRowChanged(int currentRow);
    void on_buttonCopy_clicked();
    void on_spinBox_valueChanged(int arg1);

private:
    Ui::DataCopyDialog* ui;
    const QVector<byte_t>* mData;
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
        DataString,
        DataUnicodeString,
        DataUTF8String,
        DataUCS4String,
        DataPascalByte,
        DataPascalWord,
        DataPascalDword,
        DataPascalQword,
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

    struct FormatType
    {
        QString name;
        int itemsPerLine;
    };

    FormatType mTypes[DataLast];

    void printData(DataType type);
};

#endif // DATACOPYDIALOG_H

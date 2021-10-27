#pragma once

#include <QDialog>
#include <QLineEdit>
#include "HexValidator.h"
#include "LongLongValidator.h"
#include <QDoubleValidator>

namespace Ui
{
    class EditFloatRegister;
}

class EditFloatRegister : public QDialog
{
    Q_OBJECT

public:
    explicit EditFloatRegister(int RegisterSize, QWidget* parent = 0);
    void loadData(const char* RegisterData);
    const char* getData() const;
    void selectAllText();

    ~EditFloatRegister();

private slots:
    void editingModeChangedSlot(bool arg);
    void editingHex1FinishedSlot(QString arg);
    void editingHex2FinishedSlot(QString arg);
    // the compiler does not allow us to use template as slot. So we are going to waste the screen space...
    void editingLowerShort0FinishedSlot(QString arg);
    void editingLowerShort1FinishedSlot(QString arg);
    void editingLowerShort2FinishedSlot(QString arg);
    void editingLowerShort3FinishedSlot(QString arg);
    void editingLowerShort4FinishedSlot(QString arg);
    void editingLowerShort5FinishedSlot(QString arg);
    void editingLowerShort6FinishedSlot(QString arg);
    void editingLowerShort7FinishedSlot(QString arg);
    void editingUpperShort0FinishedSlot(QString arg);
    void editingUpperShort1FinishedSlot(QString arg);
    void editingUpperShort2FinishedSlot(QString arg);
    void editingUpperShort3FinishedSlot(QString arg);
    void editingUpperShort4FinishedSlot(QString arg);
    void editingUpperShort5FinishedSlot(QString arg);
    void editingUpperShort6FinishedSlot(QString arg);
    void editingUpperShort7FinishedSlot(QString arg);
    void editingLowerLong0FinishedSlot(QString arg);
    void editingLowerLong1FinishedSlot(QString arg);
    void editingLowerLong2FinishedSlot(QString arg);
    void editingLowerLong3FinishedSlot(QString arg);
    void editingUpperLong0FinishedSlot(QString arg);
    void editingUpperLong1FinishedSlot(QString arg);
    void editingUpperLong2FinishedSlot(QString arg);
    void editingUpperLong3FinishedSlot(QString arg);
    void editingLowerFloat0FinishedSlot(QString arg);
    void editingLowerFloat1FinishedSlot(QString arg);
    void editingLowerFloat2FinishedSlot(QString arg);
    void editingLowerFloat3FinishedSlot(QString arg);
    void editingUpperFloat0FinishedSlot(QString arg);
    void editingUpperFloat1FinishedSlot(QString arg);
    void editingUpperFloat2FinishedSlot(QString arg);
    void editingUpperFloat3FinishedSlot(QString arg);
    void editingLowerDouble0FinishedSlot(QString arg);
    void editingLowerDouble1FinishedSlot(QString arg);
    void editingUpperDouble0FinishedSlot(QString arg);
    void editingUpperDouble1FinishedSlot(QString arg);
    void editingLowerLongLong0FinishedSlot(QString arg);
    void editingLowerLongLong1FinishedSlot(QString arg);
    void editingUpperLongLong0FinishedSlot(QString arg);
    void editingUpperLongLong1FinishedSlot(QString arg);

private:
    HexValidator hexValidate;
    LongLongValidator signedShortValidator;
    LongLongValidator unsignedShortValidator;
    LongLongValidator signedLongValidator;
    LongLongValidator unsignedLongValidator;
    LongLongValidator signedLongLongValidator;
    LongLongValidator unsignedLongLongValidator;
    QDoubleValidator doubleValidator;

    void hideUpperPart();
    void hideNonMMXPart();

    void reloadDataLow();
    void reloadDataHigh();

    void reloadShortData(QLineEdit & txtbox, char* Data);
    void reloadLongData(QLineEdit & txtbox, char* Data);
    void reloadFloatData(QLineEdit & txtbox, char* Data);
    void reloadDoubleData(QLineEdit & txtbox, char* Data);
    void reloadLongLongData(QLineEdit & txtbox, char* Data);

    void editingShortFinishedSlot(size_t offset, QString arg);
    void editingLongFinishedSlot(size_t offset, QString arg);
    void editingFloatFinishedSlot(size_t offset, QString arg);
    void editingDoubleFinishedSlot(size_t offset, QString arg);
    void editingLongLongFinishedSlot(size_t offset, QString arg);

    Ui::EditFloatRegister* ui;
    QObject* mutex;
    char Data[64];
    int RegSize;
};

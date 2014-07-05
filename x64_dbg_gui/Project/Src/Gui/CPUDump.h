#ifndef CPUDUMP_H
#define CPUDUMP_H

#include <QtGui>
#include <QtDebug>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include "NewTypes.h"
#include "LineEditDialog.h"
#include "HexDump.h"
#include "Bridge.h"
#include "GotoDialog.h"

class CPUDump : public HexDump
{
    Q_OBJECT
public:
    explicit CPUDump(QWidget *parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();
    void contextMenuEvent(QContextMenuEvent* event);

public slots:
    void memoryAccessSingleshootSlot();
    void memoryAccessRestoreSlot();
    void memoryWriteSingleshootSlot();
    void memoryWriteRestoreSlot();
    void memoryExecuteSingleshootSlot();
    void memoryExecuteRestoreSlot();
    void memoryRemoveSlot();
    void hardwareAccess1Slot();
    void hardwareAccess2Slot();
    void hardwareAccess4Slot();
    void hardwareAccess8Slot();
    void hardwareWrite1Slot();
    void hardwareWrite2Slot();
    void hardwareWrite4Slot();
    void hardwareWrite8Slot();
    void hardwareExecuteSlot();
    void hardwareRemoveSlot();

    void setLabelSlot();
    void gotoExpressionSlot();

    void hexAsciiSlot();
    void hexUnicodeSlot();

    void textAsciiSlot();
    void textUnicodeSlot();

    void integerSignedShortSlot();
    void integerSignedLongSlot();
    void integerSignedLongLongSlot();
    void integerUnsignedShortSlot();
    void integerUnsignedLongSlot();
    void integerUnsignedLongLongSlot();
    void integerHexShortSlot();
    void integerHexLongSlot();
    void integerHexLongLongSlot();

    void floatFloatSlot();
    void floatDoubleSlot();
    void floatLongDoubleSlot();

    void addressSlot();
    void disassemblySlot();

    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);

    void binaryEditSlot();
    void binaryFillSlot();
    void binaryCopySlot();
    void binaryPasteSlot();

private:
    QMenu* mBreakpointMenu;

    QMenu* mMemoryAccessMenu;
    QAction* mMemoryAccessSingleshoot;
    QAction* mMemoryAccessRestore;
    QMenu* mMemoryWriteMenu;
    QAction* mMemoryWriteSingleshoot;
    QAction* mMemoryWriteRestore;
    QMenu* mMemoryExecuteMenu;
    QAction* mMemoryExecuteSingleshoot;
    QAction* mMemoryExecuteRestore;
    QAction* mMemoryRemove;
    QMenu* mHardwareAccessMenu;
    QAction* mHardwareAccess1;
    QAction* mHardwareAccess2;
    QAction* mHardwareAccess4;
#ifdef _WIN64
    QAction* mHardwareAccess8;
#endif //_WIN64
    QMenu* mHardwareWriteMenu;
    QAction* mHardwareWrite1;
    QAction* mHardwareWrite2;
    QAction* mHardwareWrite4;
#ifdef _WIN64
    QAction* mHardwareWrite8;
#endif //_WIN64
    QAction* mHardwareExecute;
    QAction* mHardwareRemove;

    QMenu* mGotoMenu;
    QAction* mGotoExpression;

    QMenu* mHexMenu;
    QAction* mHexAsciiAction;
    QAction* mHexUnicodeAction;

    QMenu* mTextMenu;
    QAction* mTextAsciiAction;
    QAction* mTextUnicodeAction;

    QMenu* mIntegerMenu;
    QAction* mIntegerSignedShortAction;
    QAction* mIntegerSignedLongAction;
#ifdef _WIN64
    QAction* mIntegerSignedLongLongAction;
#endif //_WIN64
    QAction* mIntegerUnsignedShortAction;
    QAction* mIntegerUnsignedLongAction;
#ifdef _WIN64
    QAction* mIntegerUnsignedLongLongAction;
#endif //_WIN64
    QAction* mIntegerHexShortAction;
    QAction* mIntegerHexLongAction;
#ifdef _WIN64
    QAction* mIntegerHexLongLongAction;
#endif //_WIN64

    QMenu* mFloatMenu;
    QAction* mFloatFloatAction;
    QAction* mFloatDoubleAction;
    QAction* mFloatLongDoubleAction;

    QAction* mAddressAction;
    QAction* mDisassemblyAction;

    QAction* mSetLabelAction;

    QMenu* mBinaryMenu;
    QAction* mBinaryEditAction;
    QAction* mBinaryFillAction;
    QAction* mBinaryCopyAction;
    QAction* mBinaryPasteAction;

    QMenu* mSpecialMenu;
    QMenu* mCustomMenu;

    GotoDialog* mGoto;

    enum ViewEnum_t
    {
        ViewHexAscii = 0,
        ViewHexUnicode,
        ViewTextAscii,
        ViewTextUnicode,
        ViewIntegerSignedShort,
        ViewIntegerSignedLong,
        ViewIntegerSignedLongLong,
        ViewIntegerUnsignedShort,
        ViewIntegerUnsignedLong,
        ViewIntegerUnsignedLongLong,
        ViewIntegerHexShort,
        ViewIntegerHexLong,
        ViewIntegerHexLongLong,
        ViewFloatFloat,
        ViewFloatDouble,
        ViewFloatLongDouble,
        ViewAddress
    };
};

#endif // CPUDUMP_H

#pragma once

#include "HexDump.h"

//forward declaration
class CPUMultiDump;
class CPUDisassembly;
class GotoDialog;
class CommonActions;

class CPUDump : public HexDump
{
    Q_OBJECT
public:
    explicit CPUDump(CPUDisassembly* disas, CPUMultiDump* multiDump, QWidget* parent = 0);
    void getColumnRichText(int col, dsint rva, RichTextPainter::List & richText) override;
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();
    void getAttention();
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

signals:
    void displayReferencesWidget();
    void showDisassemblyTab(duint selectionStart, duint selectionEnd, duint firstAddress);

public slots:
    void modifyValueSlot();
    void gotoExpressionSlot();
    void gotoFileOffsetSlot();
    void gotoStartSlot();
    void gotoEndSlot();
    void gotoPreviousReferenceSlot();
    void gotoNextReferenceSlot();

    void hexAsciiSlot();
    void hexUnicodeSlot();
    void hexCodepageSlot();
    void hexLastCodepageSlot();

    void textAsciiSlot();
    void textUnicodeSlot();
    void textCodepageSlot();
    void textLastCodepageSlot();

    void integerSignedByteSlot();
    void integerSignedShortSlot();
    void integerSignedLongSlot();
    void integerSignedLongLongSlot();
    void integerUnsignedByteSlot();
    void integerUnsignedShortSlot();
    void integerUnsignedLongSlot();
    void integerUnsignedLongLongSlot();
    void integerHexShortSlot();
    void integerHexLongSlot();
    void integerHexLongLongSlot();

    void floatFloatSlot();
    void floatDoubleSlot();
    void floatLongDoubleSlot();

    void addressUnicodeSlot();
    void addressAsciiSlot();
    void disassemblySlot();

    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);

    void binaryEditSlot();
    void binaryFillSlot();
    void binaryCopySlot();
    void binaryPasteSlot();
    void binaryPasteIgnoreSizeSlot();
    void binarySaveToFileSlot();
    void findPattern();
    void copyFileOffsetSlot();
    void undoSelectionSlot();
    void findReferencesSlot();

    void selectionUpdatedSlot();
    void syncWithExpressionSlot();
    void allocMemorySlot();

    void headerButtonReleasedSlot(int colIndex);

private:
    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;

    QMenu* mPluginMenu;
    QMenu* mFollowInDumpMenu;
    QList<QAction*> mFollowInDumpActions;

    GotoDialog* mGoto = nullptr;
    GotoDialog* mGotoOffset = nullptr;
    CPUDisassembly* mDisas;
    CPUMultiDump* mMultiDump;
    int mAsciiSeparator = 0;

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
        ViewAddress,
        ViewIntegerSignedByte,
        ViewIntegerUnsignedByte,
        ViewAddressAscii,
        ViewAddressUnicode,
        ViewHexCodepage,
        ViewTextCodepage
    };

    void setView(ViewEnum_t view);
};

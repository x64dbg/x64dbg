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
    explicit CPUDump(CPUMultiDump* multiDump, CPUDisassembly* disasassembly, QWidget* parent = nullptr);
    void getColumnRichText(duint col, duint rva, RichTextPainter::List & richText) override;
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
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
    void floatHalfSlot();

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

    void headerButtonReleasedSlot(duint colIndex);

private:
    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;

    QMenu* mPluginMenu;
    QMenu* mFollowInDumpMenu;
    QList<QAction*> mFollowInDumpActions;

    GotoDialog* mGoto = nullptr;
    GotoDialog* mGotoOffset = nullptr;
    GotoDialog* mGotoType = nullptr;
    CPUDisassembly* mDisassembly = nullptr;
    CPUMultiDump* mMultiDump = nullptr;
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
        ViewTextCodepage,
        ViewFloatHalf
    };

    void setView(ViewEnum_t view);
};

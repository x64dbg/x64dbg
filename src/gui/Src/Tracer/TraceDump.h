#pragma once

#include "HexDump.h"
#include "TraceFileDump.h"

//forward declaration
// TODO: multiple dumps
class TraceWidget;
class TraceBrowser;
class GotoDialog;
class CommonActions;

class TraceDump : public HexDump
{
    Q_OBJECT
public:
    explicit TraceDump(Architecture* architecture, TraceWidget* parent, TraceFileDumpMemoryPage* memoryPage);
    ~TraceDump();
    void getColumnRichText(duint col, duint rva, RichTextPainter::List & richText) override;
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    void setupContextMenu();
    //void getAttention();
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void printDumpAt(duint parVA, bool select, bool repaint, bool updateTableOffset) override;

signals:
    void showDisassemblyTab(duint selectionStart, duint selectionEnd, duint firstAddress);
    void xrefSignal(duint addr);

public slots:
    void gotoExpressionSlot();
    //void gotoFileOffsetSlot();
    //void gotoStartSlot();
    //void gotoEndSlot();
    //void gotoPreviousReferenceSlot();
    //void gotoNextReferenceSlot();

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

    void binaryCopySlot();
    void binarySaveToFileSlot();
    void findPattern();
    void copyFileOffsetSlot();
    //void findReferencesSlot();

    void selectionUpdatedSlot();
    //void syncWithExpressionSlot();//TODO: Do we really need to sync with expression here?

    void gotoXrefSlot();

    void headerButtonReleasedSlot(duint colIndex);

private:
    TraceFileDumpMemoryPage* mMemoryPage;
    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;

    //QMenu* mPluginMenu;
    //QMenu* mFollowInDumpMenu;
    QList<QAction*> mFollowInDumpActions;

    GotoDialog* mGoto = nullptr;
    GotoDialog* mGotoOffset = nullptr;
    TraceWidget* mParent;
    //CPUMultiDump* mMultiDump;
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

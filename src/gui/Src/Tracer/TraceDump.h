#pragma once

#include "HexDump.h"
#include "TraceFileDump.h"

//forward declaration
//class CPUMultiDump;
class TraceBrowser;
class GotoDialog;
class CommonActions;

class TraceDump : public HexDump
{
    Q_OBJECT
public:
    explicit TraceDump(Architecture* architecture, TraceBrowser* disas, TraceFileDumpMemoryPage* memoryPage, QWidget* parent);
    ~TraceDump();
    void getColumnRichText(duint col, duint rva, RichTextPainter::List & richText) override;
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    void setupContextMenu();
    void getAttention();
    void contextMenuEvent(QContextMenuEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void printDumpAt(dsint parVA, bool select, bool repaint, bool updateTableOffset);

signals:
    void displayReferencesWidget();
    void showDisassemblyTab(duint selectionStart, duint selectionEnd, duint firstAddress);

public slots:
    void gotoExpressionSlot();
    //void gotoFileOffsetSlot();
    void gotoStartSlot();
    void gotoEndSlot();
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

    void addressUnicodeSlot();
    void addressAsciiSlot();
    void disassemblySlot();

    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);

    void binaryCopySlot();
    void binarySaveToFileSlot();
    void findPattern();
    void copyFileOffsetSlot();
    void findReferencesSlot();

    void selectionUpdatedSlot();
    void syncWithExpressionSlot();

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
    TraceBrowser* mDisas;
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
        ViewTextCodepage
    };

    void setView(ViewEnum_t view);
};

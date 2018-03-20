#ifndef _HEXDUMP_H
#define _HEXDUMP_H

#include "AbstractTableView.h"
#include "RichTextPainter.h"
#include "MemoryPage.h"
#include "VaHistory.h"
#include <QTextCodec>

class HexDump : public AbstractTableView
{
    Q_OBJECT
public:
    enum DataSize
    {
        Byte = 1,
        Word = 2,
        Dword = 4,
        Qword = 8,
        Tword = 10
    };

    enum ByteViewMode
    {
        HexByte,
        AsciiByte,
        SignedDecByte,
        UnsignedDecByte
    };

    enum WordViewMode
    {
        HexWord,
        UnicodeWord,
        SignedDecWord,
        UnsignedDecWord
    };

    enum DwordViewMode
    {
        HexDword,
        SignedDecDword,
        UnsignedDecDword,
        FloatDword //sizeof(float)=4
    };

    enum QwordViewMode
    {
        HexQword,
        SignedDecQword,
        UnsignedDecQword,
        DoubleQword //sizeof(double)=8
    };

    enum TwordViewMode
    {
        FloatTword
    };

    struct DataDescriptor
    {
        DataSize itemSize; // Items size
        union // View mode
        {
            ByteViewMode byteMode;
            WordViewMode wordMode;
            DwordViewMode dwordMode;
            QwordViewMode qwordMode;
            TwordViewMode twordMode;
        };
    };

    struct ColumnDescriptor
    {
        bool isData = true;
        int itemCount = 16;
        int separator = 0;
        QTextCodec* textCodec = nullptr; //name of the text codec (leave empty if you want to keep your sanity)
        DataDescriptor data;
        std::function<void()> columnSwitch;
    };

    explicit HexDump(QWidget* parent = 0);
    ~HexDump() override;

    // Configuration
    void updateColors() override;
    void updateFonts() override;
    void updateShortcuts() override;

    //QString getStringToPrint(int rowBase, int rowOffset, int col);
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;
    void paintGraphicDump(QPainter* painter, int x, int y, int addr);
    void printSelected(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

    // Selection Management
    void expandSelectionUpTo(dsint rva);
    void setSingleSelection(dsint rva);
    dsint getInitialSelection();
    dsint getSelectionStart();
    dsint getSelectionEnd();
    bool isSelected(dsint rva);

    virtual void getColumnRichText(int col, dsint rva, RichTextPainter::List & richText);
    int getSizeOf(DataSize size);

    void toString(DataDescriptor desc, duint rva, byte_t* data, RichTextPainter::CustomRichText_t & richText);

    void byteToString(duint rva, byte_t byte, ByteViewMode mode, RichTextPainter::CustomRichText_t & richText);
    void wordToString(duint rva, uint16 word, WordViewMode mode, RichTextPainter::CustomRichText_t & richText);
    void dwordToString(duint rva, uint32 dword, DwordViewMode mode, RichTextPainter::CustomRichText_t & richText);
    void qwordToString(duint rva, uint64 qword, QwordViewMode mode, RichTextPainter::CustomRichText_t & richText);
    void twordToString(duint rva, void* tword, TwordViewMode mode, RichTextPainter::CustomRichText_t & richText);

    int getStringMaxLength(DataDescriptor desc);

    int byteStringMaxLength(ByteViewMode mode);
    int wordStringMaxLength(WordViewMode mode);
    int dwordStringMaxLength(DwordViewMode mode);
    int qwordStringMaxLength(QwordViewMode mode);
    int twordStringMaxLength(TwordViewMode mode);

    int getItemIndexFromX(int x);
    dsint getItemStartingAddress(int x, int y);

    int getBytePerRowCount();
    int getItemPixelWidth(ColumnDescriptor desc);

    //descriptor management
    void appendDescriptor(int width, QString title, bool clickable, ColumnDescriptor descriptor);
    void appendResetDescriptor(int width, QString title, bool clickable, ColumnDescriptor descriptor);
    void clearDescriptors();

    void printDumpAt(dsint parVA, bool select, bool repaint = true, bool updateTableOffset = true);
    duint rvaToVa(dsint rva);
    duint getTableOffsetRva();
    QString makeAddrText(duint va);
    QString makeCopyText();

    void setupCopyMenu();

    VaHistory mHistory;

signals:
    void selectionUpdated();

public slots:
    void printDumpAt(dsint parVA);
    void debugStateChanged(DBGSTATE state);
    void updateDumpSlot();
    void copySelectionSlot();
    void copyAddressSlot();
    void copyRvaSlot();
    void gotoPreviousSlot();
    void gotoNextSlot();

private:
    enum GuiState
    {
        NoState,
        MultiRowsSelectionState
    };

    struct SelectionData
    {
        dsint firstSelectedIndex;
        dsint fromIndex;
        dsint toIndex;
    };

    SelectionData mSelection;

    GuiState mGuiState;
    QChar mNonprintReplace;
    QChar mNullReplace;

    QColor mModifiedBytesColor;
    QColor mModifiedBytesBackgroundColor;
    QColor mRestoredBytesColor;
    QColor mRestoredBytesBackgroundColor;
    QColor mByte00Color;
    QColor mByte00BackgroundColor;
    QColor mByte7FColor;
    QColor mByte7FBackgroundColor;
    QColor mByteFFColor;
    QColor mByteFFBackgroundColor;
    QColor mByteIsPrintColor;
    QColor mByteIsPrintBackgroundColor;

    QColor mUserModuleCodePointerHighlightColor;
    QColor mUserModuleDataPointerHighlightColor;
    QColor mSystemModuleCodePointerHighlightColor;
    QColor mSystemModuleDataPointerHighlightColor;
    QColor mUnknownCodePointerHighlightColor;
    QColor mUnknownDataPointerHighlightColor;

protected:
    MemoryPage* mMemPage;
    int mByteOffset;
    QList<ColumnDescriptor> mDescriptor;
    int mForceColumn;
    bool mRvaDisplayEnabled;
    duint mRvaDisplayBase;
    dsint mRvaDisplayPageBase;
    QString mSyncAddrExpression;
    QAction* mCopyAddress;
    QAction* mCopyRva;
    QAction* mCopySelection;
};

#endif // _HEXDUMP_H

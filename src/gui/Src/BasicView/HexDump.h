#pragma once

#include "AbstractTableView.h"
#include <Utils/RichTextPainter.h>
#include <Memory/MemoryPage.h>
#include <Utils/VaHistory.h>
#include <Disassembler/Architecture.h>

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
        UnsignedDecWord,
        HalfFloatWord //half precision floatint point
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
        QByteArray textEncoding; // name of the text codec (leave empty if you want to keep your sanity)
        DataDescriptor data;
        std::function<void()> columnSwitch;
    };

    explicit HexDump(Architecture* architecture, QWidget* parent = nullptr, MemoryPage* memPage = nullptr);
    ~HexDump() override;

    // Configuration
    void updateColors() override;
    void updateFonts() override;
    void updateShortcuts() override;

    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    QString paintContent(QPainter* painter, duint row, duint column, int x, int y, int w, int h) override;
    void paintGraphicDump(QPainter* painter, int x, int y, int addr);
    void printSelected(QPainter* painter, duint row, duint column, int x, int y, int w, int h);

    // Selection Management
    void expandSelectionUpTo(duint rva);
    void setSingleSelection(duint rva);
    duint getInitialSelection() const;
    duint getSelectionStart() const;
    duint getSelectionEnd() const;
    bool isSelected(duint rva) const;

    virtual void getColumnRichText(duint column, duint rva, RichTextPainter::List & richText);

    static size_t getSizeOf(DataSize size);

    void toString(DataDescriptor desc, duint rva, uint8_t* data, RichTextPainter::CustomRichText_t & richText);

    void byteToString(duint rva, uint8_t byte, ByteViewMode mode, RichTextPainter::CustomRichText_t & richText);
    void wordToString(duint rva, uint16_t word, WordViewMode mode, RichTextPainter::CustomRichText_t & richText);
    static void dwordToString(duint rva, uint32_t dword, DwordViewMode mode, RichTextPainter::CustomRichText_t & richText);
    static void qwordToString(duint rva, uint64_t qword, QwordViewMode mode, RichTextPainter::CustomRichText_t & richText);
    static void twordToString(duint rva, void* tword, TwordViewMode mode, RichTextPainter::CustomRichText_t & richText);

    int getItemIndexFromX(int x) const;
    duint getItemStartingAddress(int x, int y);

    size_t getBytePerRowCount() const;
    int getItemPixelWidth(ColumnDescriptor desc) const;

    //descriptor management
    void appendDescriptor(int width, QString title, bool clickable, ColumnDescriptor descriptor);
    void appendResetDescriptor(int width, QString title, bool clickable, ColumnDescriptor descriptor);
    void clearDescriptors();

    virtual void printDumpAt(duint parVA, bool select, bool repaint = true, bool updateTableOffset = true);
    duint rvaToVa(duint rva) const;

    duint getTableOffsetRva() const;
    QString makeAddrText(duint va) const;
    QString makeCopyText();

    void setupCopyMenu();

    VaHistory mHistory;

signals:
    void selectionUpdated();

public slots:
    void printDumpAt(duint parVA);
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
        duint firstSelectedIndex = 0;
        duint fromIndex = 0;
        duint toIndex = 0;
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

    struct UpdateCache
    {
        duint memBase = 0;
        duint memSize = 0;
        duint rva = 0;
        duint size = 0;

        bool operator==(const UpdateCache & o) const
        {
            return std::tie(memBase, memSize, rva, size) == std::tie(o.memBase, o.memSize, o.rva, o.size);
        }
    } mUpdateCache;
    std::vector<uint8_t> mUpdateCacheData;
    std::vector<uint8_t> mUpdateCacheTemp;

protected:
    Architecture* mArchitecture = nullptr;
    MemoryPage* mMemPage = nullptr;
    dsint mByteOffset = 0;
    QList<ColumnDescriptor> mDescriptor;
    int mForceColumn;
    bool mRvaDisplayEnabled;
    duint mRvaDisplayBase;
    duint mRvaDisplayPageBase;
    QString mSyncAddrExpression;
    QAction* mCopyAddress;
    QAction* mCopyRva;
    QAction* mCopySelection;
    duint mUnderlineRangeStartVa = 0;
    duint mUnderlineRangeEndVa = 0;
    bool mUnderliningEnabled = true;
};

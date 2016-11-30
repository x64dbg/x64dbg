#ifndef _HEXDUMP_H
#define _HEXDUMP_H

#include "AbstractTableView.h"
#include "RichTextPainter.h"
#include "MemoryPage.h"
#include <QTextCodec>

class HexDump : public AbstractTableView
{
    Q_OBJECT
public:
    enum DataSize_e
    {
        Byte,
        Word,
        Dword,
        Qword,
        Tword
    };

    enum ByteViewMode_e
    {
        HexByte,
        AsciiByte,
        SignedDecByte,
        UnsignedDecByte
    };

    enum WordViewMode_e
    {
        HexWord,
        UnicodeWord,
        SignedDecWord,
        UnsignedDecWord
    };

    enum DwordViewMode_e
    {
        HexDword,
        SignedDecDword,
        UnsignedDecDword,
        FloatDword //sizeof(float)=4
    };

    enum QwordViewMode_e
    {
        HexQword,
        SignedDecQword,
        UnsignedDecQword,
        DoubleQword //sizeof(double)=8
    };

    enum TwordViewMode_e
    {
        FloatTword
    };

    typedef struct _DataDescriptor_t
    {
        DataSize_e itemSize; // Items size
        union // View mode
        {
            ByteViewMode_e byteMode;
            WordViewMode_e wordMode;
            DwordViewMode_e dwordMode;
            QwordViewMode_e qwordMode;
            TwordViewMode_e twordMode;
        };
    } DataDescriptor_t;

    struct ColumnDescriptor_t
    {
        bool isData = true;
        int itemCount = 16;
        int separator = 0;
        QTextCodec* textCodec; //name of the text codec (leave empty if you want to keep your sanity)
        DataDescriptor_t data;

        explicit ColumnDescriptor_t()
            : textCodec(nullptr)
        {
        }
    };

    explicit HexDump(QWidget* parent = 0);
    virtual ~HexDump();

    // Configuration
    virtual void updateColors();
    virtual void updateFonts();
    virtual void updateShortcuts();

    //QString getStringToPrint(int rowBase, int rowOffset, int col);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);
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
    int getSizeOf(DataSize_e size);

    QString toString(DataDescriptor_t desc, void* data);

    QString byteToString(byte_t byte, ByteViewMode_e mode);
    QString wordToString(uint16 word, WordViewMode_e mode);
    QString dwordToString(uint32 dword, DwordViewMode_e mode);
    QString qwordToString(uint64 qword, QwordViewMode_e mode);
    QString twordToString(void* tword, TwordViewMode_e mode);

    int getStringMaxLength(DataDescriptor_t desc);

    int byteStringMaxLength(ByteViewMode_e mode);
    int wordStringMaxLength(WordViewMode_e mode);
    int dwordStringMaxLength(DwordViewMode_e mode);
    int qwordStringMaxLength(QwordViewMode_e mode);
    int twordStringMaxLength(TwordViewMode_e mode);

    int getItemIndexFromX(int x);
    dsint getItemStartingAddress(int x, int y);

    int getBytePerRowCount();
    int getItemPixelWidth(ColumnDescriptor_t desc);

    //descriptor management
    void appendDescriptor(int width, QString title, bool clickable, ColumnDescriptor_t descriptor);
    void appendResetDescriptor(int width, QString title, bool clickable, ColumnDescriptor_t descriptor);
    void clearDescriptors();

    void printDumpAt(dsint parVA, bool select, bool repaint = true, bool updateTableOffset = true);
    duint rvaToVa(dsint rva);
    duint getTableOffsetRva();
    QString makeAddrText(duint va);
    QString makeCopyText();

    void addVaToHistory(dsint parVa);
    bool historyHasPrev();
    bool historyHasNext();
    void historyPrev();
    void historyNext();
    void historyClear();

    void setupCopyMenu();

signals:
    void selectionUpdated();

public slots:
    void printDumpAt(dsint parVA);
    void debugStateChanged(DBGSTATE state);
    void updateDumpSlot();
    void copySelectionSlot();
    void copyAddressSlot();
    void copyRvaSlot();

private:
    enum GuiState_t {NoState, MultiRowsSelectionState};

    typedef struct _RowDescriptor_t
    {
        dsint firstSelectedIndex;
        dsint fromIndex;
        dsint toIndex;
    } SelectionData_t;

    SelectionData_t mSelection;

    GuiState_t mGuiState;

    QList<dsint> mVaHistory;
    int mCurrentVa;
    QChar mNonprintReplace;
    QChar mNullReplace;

protected:
    MemoryPage* mMemPage;
    int mByteOffset;
    QList<ColumnDescriptor_t> mDescriptor;
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

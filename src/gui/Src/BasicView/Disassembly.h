#ifndef DISASSEMBLY_H
#define DISASSEMBLY_H

#include "AbstractTableView.h"
#include "DisassemblyPopup.h"

class CodeFoldingHelper;
class QBeaEngine;
class MemoryPage;

class Disassembly : public AbstractTableView
{
    Q_OBJECT
public:
    explicit Disassembly(QWidget* parent = 0);
    ~Disassembly() override;

    // Configuration
    void updateColors() override;
    void updateFonts() override;

    // Reimplemented Functions
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;

    // Mouse Management
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

    // Keyboard Management
    void keyPressEvent(QKeyEvent* event) override;

    // ScrollBar Management
    dsint sliderMovedHook(int type, dsint value, dsint delta) override;

    // Jumps Graphic
    int paintJumpsGraphic(QPainter* painter, int x, int y, dsint addr, bool isjmp);

    // Function Graphic

    enum Function_t
    {
        Function_none,
        Function_single,
        Function_start,
        Function_middle,
        Function_loop_entry,
        Function_end
    };

    int paintFunctionGraphic(QPainter* painter, int x, int y, Function_t funcType, bool loop);

    // Instructions Management
    dsint getPreviousInstructionRVA(dsint rva, duint count);
    dsint getNextInstructionRVA(dsint rva, duint count, bool isGlobal = false);
    dsint getInstructionRVA(dsint index, dsint count);
    Instruction_t DisassembleAt(dsint rva);
    Instruction_t DisassembleAt(dsint rva, dsint count);

    // Selection Management
    void expandSelectionUpTo(dsint to);
    void setSingleSelection(dsint index);
    dsint getInitialSelection();
    dsint getSelectionSize();
    dsint getSelectionStart();
    dsint getSelectionEnd();
    void selectNext(bool expand);
    void selectPrevious(bool expand);
    bool isSelected(dsint base, dsint offset);
    bool isSelected(QList<Instruction_t>* buffer, int index);
    duint getSelectedVa();

    // Update/Reload/Refresh/Repaint
    void prepareData() override;
    void reloadData() override;

    // Public Methods
    duint rvaToVa(dsint rva) const;
    void disassembleClear();
    const duint getBase() const;
    duint getSize();
    duint getTableOffsetRva();

    // history management
    void historyClear();
    void historyPrevious();
    void historyNext();
    bool historyHasPrevious();
    bool historyHasNext();

    //disassemble
    void disassembleAt(dsint parVA, dsint parCIP, bool history, dsint newTableOffset);

    QList<Instruction_t>* instructionsBuffer(); // ugly
    const dsint baseAddress() const;
    const dsint currentEIP() const;

    QString getAddrText(dsint cur_addr, char label[MAX_LABEL_SIZE], bool getLabel = true);
    void prepareDataCount(const QList<dsint> & wRVAs, QList<Instruction_t>* instBuffer);
    void prepareDataRange(dsint startRva, dsint endRva, const std::function<bool(int, const Instruction_t &)> & disassembled);
    RichTextPainter::List getRichBytes(const Instruction_t & instr) const;

    //misc
    void setCodeFoldingManager(CodeFoldingHelper* CodeFoldingManager);
    void unfold(dsint rva);
    void ShowDisassemblyPopup(duint addr, int x, int y);
    bool hightlightToken(const CapstoneTokenizer::SingleToken & token);
    bool isHighlightMode();

signals:
    void selectionChanged(dsint parVA);
    void selectionExpanded();
    void disassembledAt(dsint parVA, dsint parCIP, bool history, dsint newTableOffset);
    void updateWindowTitle(QString title);

public slots:
    void disassembleAt(dsint parVA, dsint parCIP);
    void debugStateChangedSlot(DBGSTATE state);
    void selectionChangedSlot(dsint parVA);
    void tokenizerConfigUpdatedSlot();

private:
    enum GuiState
    {
        NoState,
        MultiRowsSelectionState
    };

    enum GraphicDump
    {
        GD_Nothing,
        GD_FootToTop, //GD_FootToTop = '-
        GD_FootToBottom, //GD_FootToBottom = ,-
        GD_HeadFromTop, //GD_HeadFromTop = '->
        GD_HeadFromBottom, //GD_HeadFromBottom = ,->
        GD_HeadFromBoth, //GD_HeadFromBoth = |->
        GD_Vert, //GD_Vert = |
        GD_VertHori //GD_VertHori = |-
    };

    enum GraphicJumpDirection
    {
        GJD_Nothing,
        GJD_Up,
        GJD_Down
    };

    struct SelectionData
    {
        dsint firstSelectedIndex;
        dsint fromIndex;
        dsint toIndex;
    };

    SelectionData mSelection;

    bool mIsLastInstDisplayed;

    GuiState mGuiState;

    dsint mCipRva;

    QList<Instruction_t> mInstBuffer;

    struct HistoryData
    {
        dsint va;
        dsint tableOffset;
        QString windowTitle;
    };

    QList<HistoryData> mVaHistory;
    int mCurrentVa;

protected:
    // Configuration
    QColor mInstructionHighlightColor;
    QColor mSelectionColor;
    QColor mDisassemblyRelocationUnderlineColor;

    QColor mCipBackgroundColor;
    QColor mCipColor;

    QColor mBreakpointBackgroundColor;
    QColor mBreakpointColor;

    QColor mHardwareBreakpointBackgroundColor;
    QColor mHardwareBreakpointColor;

    QColor mBookmarkBackgroundColor;
    QColor mBookmarkColor;

    QColor mLabelColor;
    QColor mLabelBackgroundColor;

    QColor mSelectedAddressBackgroundColor;
    QColor mTracedAddressBackgroundColor;
    QColor mSelectedAddressColor;
    QColor mAddressBackgroundColor;
    QColor mAddressColor;
    QColor mTracedSelectedAddressBackgroundColor;

    QColor mBytesColor;
    QColor mBytesBackgroundColor;
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

    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;

    QColor mMnemonicBriefColor;
    QColor mMnemonicBriefBackgroundColor;

    QColor mCommentColor;
    QColor mCommentBackgroundColor;

    QColor mUnconditionalJumpLineColor;
    QColor mConditionalJumpLineTrueColor;
    QColor mConditionalJumpLineFalseColor;

    QColor mLoopColor;
    QColor mFunctionColor;

    QPen mLoopPen;
    QPen mFunctionPen;
    QPen mUnconditionalPen;
    QPen mConditionalTruePen;
    QPen mConditionalFalsePen;

    // Misc
    bool mRvaDisplayEnabled;
    duint mRvaDisplayBase;
    dsint mRvaDisplayPageBase;
    bool mHighlightingMode;
    bool mPopupEnabled;
    MemoryPage* mMemPage;
    QBeaEngine* mDisasm;
    bool mShowMnemonicBrief;
    XREF_INFO mXrefInfo;
    CodeFoldingHelper* mCodeFoldingManager;
    DisassemblyPopup mDisassemblyPopup;
    CapstoneTokenizer::SingleToken mHighlightToken;
    bool mPermanentHighlightingMode;
};

#endif // DISASSEMBLY_H

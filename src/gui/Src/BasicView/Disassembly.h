#ifndef DISASSEMBLY_H
#define DISASSEMBLY_H

#include "AbstractTableView.h"
#include "QBeaEngine.h"

class CodeFoldingHelper;
class MemoryPage;

enum AddrDisplayType
{
    Display_addr,
    Display_rva,
    Display_module,
};
class Disassembly : public AbstractTableView
{
    Q_OBJECT
public:
    Disassembly(QWidget* parent, bool isMain);
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

    // Keyboard Management
    void keyPressEvent(QKeyEvent* event) override;

    // ScrollBar Management
    dsint sliderMovedHook(int type, dsint value, dsint delta) override;

    // Instructions Management
    dsint getPreviousInstructionRVA(dsint rva, duint count);
    dsint getNextInstructionRVA(dsint rva, duint count, bool isGlobal = false);
    dsint getInstructionRVA(dsint index, dsint count);
    Instruction_t DisassembleAt(dsint rva);
    Instruction_t DisassembleAt(dsint rva, dsint count);

    // Selection Management
    void expandSelectionUpTo(dsint to);
    void setSingleSelection(dsint index);
    dsint getInitialSelection() const;
    dsint getSelectionSize() const;
    dsint getSelectionStart() const;
    dsint getSelectionEnd() const;
    void selectNext(bool expand);
    void selectPrevious(bool expand);
    bool isSelected(dsint base, dsint offset);
    bool isSelected(QList<Instruction_t>* buffer, int index) const;
    duint getSelectedVa() const;

    // Update/Reload/Refresh/Repaint
    void prepareData() override;
    void reloadData() override;

    // Public Methods
    duint rvaToVa(dsint rva) const;
    void disassembleClear();
    const duint getBase() const;
    duint getSize() const;
    duint getTableOffsetRva() const;

    // history management
    void historyClear();
    void historyPrevious();
    void historyNext();
    bool historyHasPrevious() const;
    bool historyHasNext() const;

    //disassemble
    void gotoAddress(duint addr);
    void disassembleAt(dsint parVA, bool history, dsint newTableOffset);

    QList<Instruction_t>* instructionsBuffer(); // ugly
    const dsint baseAddress() const;

    QString getAddrText(dsint cur_addr, char label[MAX_LABEL_SIZE], bool getLabel = true);
    void prepareDataCount(const QList<dsint> & wRVAs, QList<Instruction_t>* instBuffer);
    void prepareDataRange(dsint startRva, dsint endRva, const std::function<bool(int, const Instruction_t &)> & disassembled);
    RichTextPainter::List getRichBytes(const Instruction_t & instr, bool isSelected) const;

    //misc
    void setCodeFoldingManager(CodeFoldingHelper* CodeFoldingManager);
    duint getDisassemblyPopupAddress(int mousex, int mousey) override;
    void unfold(dsint rva);
    bool hightlightToken(const ZydisTokenizer::SingleToken & token);
    bool isHighlightMode() const;

signals:
    void selectionChanged(dsint parVA);
    void selectionExpanded();
    void updateWindowTitle(QString title);

public slots:
    void disassembleAtSlot(dsint parVA, dsint parCIP);
    void debugStateChangedSlot(DBGSTATE state);
    void selectionChangedSlot(dsint parVA);
    void tokenizerConfigUpdatedSlot();
    void updateConfigSlot();

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

    duint mCipVa = 0;

    QList<Instruction_t> mInstBuffer;

    struct HistoryData
    {
        dsint va;
        dsint tableOffset;
    };

    QList<HistoryData> mVaHistory;
    int mCurrentVa;

protected:
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
    // Configuration
    QColor mInstructionHighlightColor;
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
    //bool mRvaDisplayEnabled;
    AddrDisplayType mDisplayType;

    duint mRvaDisplayBase;
    dsint mRvaDisplayPageBase;
    bool mHighlightingMode;
    MemoryPage* mMemPage;
    QBeaEngine* mDisasm;
    bool mShowMnemonicBrief;
    XREF_INFO mXrefInfo;
    CodeFoldingHelper* mCodeFoldingManager;
    ZydisTokenizer::SingleToken mHighlightToken;
    bool mPermanentHighlightingMode;
    bool mNoCurrentModuleText;
    bool mIsMain = false;
};

#endif // DISASSEMBLY_H

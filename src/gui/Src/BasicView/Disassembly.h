#pragma once

#include "AbstractTableView.h"
#include "QZydis.h"
#include <QTextLayout>
#include "Architecture.h"

class CodeFoldingHelper;
class MemoryPage;
class DisassemblyPopup;

class Disassembly : public AbstractTableView
{
    Q_OBJECT
public:
    Disassembly(Architecture* architecture, bool isMain, QWidget* parent = nullptr);
    ~Disassembly() override;
    Architecture* getArchitecture() const;

    // Configuration
    void updateColors() override;
    void updateFonts() override;

    // Reimplemented Functions
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;

    // Mouse Management
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // Keyboard Management
    void keyPressEvent(QKeyEvent* event) override;

    // ScrollBar Management
    duint sliderMovedHook(QScrollBar::SliderAction action, duint value, dsint delta) override;

    // Instructions Management
    duint getPreviousInstructionRVA(duint rva, duint count);
    duint getNextInstructionRVA(duint rva, duint count, bool isGlobal = false);
    duint getInstructionRVA(duint index, dsint count);
    Instruction_t DisassembleAt(duint rva);
    Instruction_t DisassembleAt(duint rva, dsint count);

    // Selection Management
    void expandSelectionUpTo(duint to);
    void setSingleSelection(duint index);
    duint getInitialSelection() const;
    duint getSelectionSize() const;
    duint getSelectionStart() const;
    duint getSelectionEnd() const;
    void selectNext(bool expand);
    void selectPrevious(bool expand);
    bool isSelected(duint base, dsint offset);
    bool isSelected(QList<Instruction_t>* buffer, int index) const;
    duint getSelectedVa() const;

    // Update/Reload/Refresh/Repaint
    void prepareData() override;
    void reloadData() override;

    void paintEvent(QPaintEvent* event) override;

    // Public Methods
    duint rvaToVa(duint rva) const;
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
    void disassembleAt(duint va, bool history, duint newTableOffset);

    QList<Instruction_t>* instructionsBuffer(); // ugly
    const duint baseAddress() const;

    QString getAddrText(duint cur_addr, QString & label, bool getLabel = true);
    void prepareDataCount(const QList<duint> & rvas, QList<Instruction_t>* instBuffer);
    void prepareDataRange(duint startRva, duint endRva, const std::function<bool(int, const Instruction_t &)> & disassembled);
    RichTextPainter::List getRichBytes(const Instruction_t & instr, bool isSelected) const;

    //misc
    void setCodeFoldingManager(CodeFoldingHelper* CodeFoldingManager);
    void unfold(duint rva);
    bool hightlightToken(const ZydisTokenizer::SingleToken & token);
    bool isHighlightMode() const;

signals:
    void selectionChanged(duint va);
    void selectionExpanded();
    void updateWindowTitle(QString title);

public slots:
    void disassembleAtSlot(duint va, duint cip);
    void debugStateChangedSlot(DBGSTATE state);
    void selectionChangedSlot(duint va);
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
        GJD_Down,
        GJD_Out,
    };

    struct SelectionData
    {
        duint firstSelectedIndex = 0;
        duint fromIndex = 0;
        duint toIndex = 0;
    } mSelection;

    Architecture* mArchitecture = nullptr;
    bool mIsLastInstDisplayed;

    GuiState mGuiState;

    duint mCipVa = 0;

    Instruction_t mSelectedInstruction;
    QList<Instruction_t> mInstBuffer;

    struct HistoryData
    {
        duint va = 0;
        duint tableOffset = 0;
    };

    QList<HistoryData> mVaHistory;
    int mCurrentVa;

    enum
    {
        ColAddress,
        ColBytes,
        ColDisassembly,
        ColComment,
        ColMnemonicBrief,
    };

    DisassemblyPopup* mDisassemblyPopup = nullptr;

protected:
    // Jumps Graphic
    int paintJumpsGraphic(QPainter* painter, int x, int y, const Instruction_t & instruction);

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

    duint getAddressForPosition(int mousex, int mousey) override;

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
    bool mRvaDisplayEnabled;
    duint mRvaDisplayBase;
    dsint mRvaDisplayPageBase;
    bool mHighlightingMode;
    MemoryPage* mMemPage;
    QZydis* mDisasm;
    bool mShowMnemonicBrief;
    XREF_INFO mXrefInfo;
    CodeFoldingHelper* mCodeFoldingManager;
    ZydisTokenizer::SingleToken mHighlightToken;
    bool mPermanentHighlightingMode;
    bool mNoCurrentModuleText;
    bool mIsMain = false;

    struct RichTextInfo
    {
        bool alive = true;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        int xinc = 0;
        RichTextPainter::List richText;
    };

    QTextLayout mTextLayout;
    std::vector<QTextLayout::FormatRange> mFormatCache;
    std::vector<std::vector<RichTextInfo>> mRichText;

    void paintRichText(int x, int y, int w, int h, int xinc, const RichTextPainter::List & richText, int rowOffset, int column);
    void paintRichText(int x, int y, int w, int h, int xinc, RichTextPainter::List && richText, int rowOffset, int column);
};

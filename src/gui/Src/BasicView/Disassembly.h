#ifndef DISASSEMBLY_H
#define DISASSEMBLY_H

#include "AbstractTableView.h"
#include "QBeaEngine.h"
#include "MemoryPage.h"

class Disassembly : public AbstractTableView
{
    Q_OBJECT
public:
    explicit Disassembly(QWidget* parent = 0);

    // Configuration
    virtual void updateColors();
    virtual void updateFonts();

    // Reimplemented Functions
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

    // Mouse Management
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    // Keyboard Management
    void keyPressEvent(QKeyEvent* event);

    // ScrollBar Management
    dsint sliderMovedHook(int type, dsint value, dsint delta);

    // Jumps Graphic
    int paintJumpsGraphic(QPainter* painter, int x, int y, dsint addr);

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
    dsint getNextInstructionRVA(dsint rva, duint count);
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
    void prepareData();
    void reloadData();

    // Public Methods
    duint rvaToVa(dsint rva);
    void disassembleClear();
    const dsint getBase() const;
    dsint getSize();
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

    QString getAddrText(dsint cur_addr, char label[MAX_LABEL_SIZE]);
    void prepareDataCount(dsint wRVA, int wCount, QList<Instruction_t>* instBuffer);
    void prepareDataRange(dsint startRva, dsint endRva, QList<Instruction_t>* instBuffer);

signals:
    void selectionChanged(dsint parVA);
    void disassembledAt(dsint parVA, dsint parCIP, bool history, dsint newTableOffset);
    void updateWindowTitle(QString title);

public slots:
    void disassembleAt(dsint parVA, dsint parCIP);
    void debugStateChangedSlot(DBGSTATE state);

private:
    enum GuiState_t {NoState, MultiRowsSelectionState};
    enum GraphicDump_t {GD_Nothing, GD_FootToTop, GD_FootToBottom, GD_HeadFromTop, GD_HeadFromBottom, GD_Vert}; // GD_FootToTop = '- , GD_FootToBottom = ,- , GD_HeadFromTop = '-> , GD_HeadFromBottom = ,-> , GD_Vert = |

    typedef struct _SelectionData_t
    {
        dsint firstSelectedIndex;
        dsint fromIndex;
        dsint toIndex;
    } SelectionData_t;

    QBeaEngine* mDisasm;

    SelectionData_t mSelection;

    bool mIsLastInstDisplayed;
    bool mIsRunning;

    GuiState_t mGuiState;

    dsint mCipRva;

    QList<Instruction_t> mInstBuffer;

    typedef struct _HistoryData_t
    {
        dsint va;
        dsint tableOffset;
        QString windowTitle;
    } HistoryData_t;

    QList<HistoryData_t> mVaHistory;
    int mCurrentVa;
    CapstoneTokenizer::SingleToken mHighlightToken;

protected:
    // Configuration
    QColor mInstructionHighlightColor;
    QColor mSelectionColor;

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
    QColor mSelectedAddressColor;
    QColor mAddressBackgroundColor;
    QColor mAddressColor;

    QColor mBytesColor;
    QColor mModifiedBytesColor;
    QColor mRestoredBytesColor;

    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;

    QColor mCommentColor;
    QColor mCommentBackgroundColor;

    QColor mUnconditionalJumpLineColor;
    QColor mConditionalJumpLineTrueColor;
    QColor mConditionalJumpLineFalseColor;

    QColor mLoopColor;
    QColor mFunctionColor;

    // Misc
    bool mRvaDisplayEnabled;
    duint mRvaDisplayBase;
    dsint mRvaDisplayPageBase;
    bool mHighlightingMode;
    MemoryPage* mMemPage;
};

#endif // DISASSEMBLY_H

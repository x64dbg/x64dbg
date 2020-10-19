#ifndef TRACEBROWSER_H
#define TRACEBROWSER_H

#include "AbstractTableView.h"
#include "VaHistory.h"
#include "QBeaEngine.h"

class TraceFileReader;
class BreakpointMenu;
class MRUList;
class CommonActions;

class TraceBrowser : public AbstractTableView
{
    Q_OBJECT
public:
    explicit TraceBrowser(QWidget* parent = 0);
    ~TraceBrowser() override;

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;

    void prepareData() override;
    void updateColors() override;

    void expandSelectionUpTo(duint to);
    void setSingleSelection(duint index);
    duint getInitialSelection();
    duint getSelectionSize();
    duint getSelectionStart();
    duint getSelectionEnd();

    bool isFileOpened() const;
    TraceFileReader* getTraceFile() { return mTraceFile; }

private:
    enum TableColumnIndex
    {
        Index,
        Address,
        Opcode,
        Disassembly,
        Registers,
        Memory,
        Comments
    };
    void setupRightClickContextMenu();
    void makeVisible(duint index);
    QString getAddrText(dsint cur_addr, char label[MAX_LABEL_SIZE], bool getLabel);
    RichTextPainter::List getRichBytes(const Instruction_t & instr) const;
    void pushSelectionInto(bool copyBytes, QTextStream & stream, QTextStream* htmlStream = nullptr);
    void copySelectionSlot(bool copyBytes);
    void copySelectionToFileSlot(bool copyBytes);

    void contextMenuEvent(QContextMenuEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    ZydisTokenizer::InstructionToken memoryTokens(unsigned long long atIndex);
    ZydisTokenizer::InstructionToken registersTokens(unsigned long long atIndex);
    VaHistory mHistory;
    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;
    bool mRvaDisplayEnabled;
    duint mRvaDisplayBase;

    struct SelectionData
    {
        duint firstSelectedIndex;
        duint fromIndex;
        duint toIndex;
    };

    SelectionData mSelection;
    ZydisTokenizer::SingleToken mHighlightToken;
    bool mHighlightingMode;
    bool mPermanentHighlightingMode;
    bool mAutoDisassemblyFollowSelection;
    bool mShowMnemonicBrief;

    TraceFileReader* mTraceFile;
    BreakpointMenu* mBreakpointMenu;
    MRUList* mMRUList;
    QString mFileName;

    QColor mBytesColor;
    QColor mBytesBackgroundColor;

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
    QColor mTracedAddressBackgroundColor;
    QColor mSelectedAddressColor;
    QColor mAddressBackgroundColor;
    QColor mAddressColor;
    QColor mTracedSelectedAddressBackgroundColor;

    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;
    QColor mCommentColor;
    QColor mCommentBackgroundColor;
    QColor mDisassemblyRelocationUnderlineColor;

    QColor mMnemonicBriefColor;
    QColor mMnemonicBriefBackgroundColor;

    QColor mConditionalJumpLineTrueColor;

    QColor mLoopColor;
    QColor mFunctionColor;

    QPen mLoopPen;
    QPen mFunctionPen;
    QPen mConditionalTruePen;

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

signals:
    void displayReferencesWidget();
    void displayLogWidget();
    void selectionChanged(unsigned long long selection);

public slots:
    void openFileSlot();
    void openSlot(const QString & fileName);
    void toggleRunTraceSlot();
    void closeFileSlot();
    void closeDeleteSlot();
    void parseFinishedSlot();
    void tokenizerConfigUpdatedSlot();
    void onSelectionChanged(unsigned long long selection);

    void gotoSlot();
    void gotoPreviousSlot();
    void gotoNextSlot();
    //void followDisassemblySlot();
    void enableHighlightingModeSlot();
    void setLabelSlot();
    //void setCommentSlot();
    //void setBookmarkSlot();
    void mnemonicBriefSlot();
    void mnemonicHelpSlot();
    void copyDisassemblySlot();
    void copyCipSlot();
    void copyIndexSlot();
    void copySelectionSlot();
    void copySelectionNoBytesSlot();
    void copySelectionToFileSlot();
    void copySelectionToFileNoBytesSlot();
    void copyFileOffsetSlot();
    void copyRvaSlot();
    void exportSlot();

    void searchConstantSlot();
    void searchMemRefSlot();

    void updateSlot();

    void toggleAutoDisassemblyFollowSelectionSlot();
};

#endif //TRACEBROWSER_H

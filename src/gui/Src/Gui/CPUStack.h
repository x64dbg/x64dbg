#ifndef CPUSTACK_H
#define CPUSTACK_H

#include "HexDump.h"

//forward declaration
class CPUMultiDump;
class GotoDialog;

class CPUStack : public HexDump
{
    Q_OBJECT
public:
    explicit CPUStack(CPUMultiDump* multiDump, QWidget* parent = 0);

    // Configuration
    virtual void updateColors();
    virtual void updateFonts();

    void getColumnRichText(int col, dsint rva, RichTextPainter::List & richText) override;
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;
    void contextMenuEvent(QContextMenuEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void setupContextMenu();
    void updateFreezeStackAction();

signals:
    void displayReferencesWidget();

public slots:
    void pushSlot();
    void popSlot();
    void refreshShortcutsSlot();
    void stackDumpAt(duint addr, duint csp);
    void gotoSpSlot();
    void gotoBpSlot();
    void gotoExpressionSlot();
    void gotoPreviousSlot();
    void gotoNextSlot();
    void gotoPreviousFrameSlot();
    void gotoNextFrameSlot();
    void gotoFrameBaseSlot();
    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);
    void selectionUpdatedSlot();
    void followDisasmSlot();
    void followDumpPtrSlot();
    void followinDumpNSlot();
    void followStackSlot();
    void watchDataSlot();
    void binaryEditSlot();
    void binaryFillSlot();
    void binaryCopySlot();
    void binaryPasteSlot();
    void memoryAccessSingleshootSlot();
    void memoryAccessRestoreSlot();
    void memoryWriteSingleshootSlot();
    void memoryWriteRestoreSlot();
    void memoryRemoveSlot();
    void hardwareAccess1Slot();
    void hardwareAccess2Slot();
    void hardwareAccess4Slot();
    void hardwareAccess8Slot();
    void hardwareWrite1Slot();
    void hardwareWrite2Slot();
    void hardwareWrite4Slot();
    void hardwareWrite8Slot();
    void hardwareRemoveSlot();
    void findPattern();
    void binaryPasteIgnoreSizeSlot();
    void undoSelectionSlot();
    void modifySlot();
    void realignSlot();
    void freezeStackSlot();
    void dbgStateChangedSlot(DBGSTATE state);
    void followInMemoryMapSlot();
    void followInDumpSlot();

private:
    duint mCsp;
    bool bStackFrozen;

    QMenu* mBinaryMenu;
    QAction* mBinaryEditAction;
    QAction* mBinaryFillAction;
    QAction* mBinaryCopyAction;
    QAction* mBinaryPasteAction;
    QAction* mBinaryPasteIgnoreSizeAction;
    QMenu* mBreakpointMenu;
    QMenu* mBreakpointHardwareAccessMenu;
    QAction* mBreakpointHardwareAccess1;
    QAction* mBreakpointHardwareAccess2;
    QAction* mBreakpointHardwareAccess4;
#ifdef _WIN64
    QAction* mBreakpointHardwareAccess8;
#endif //_WIN64
    QMenu* mBreakpointHardwareWriteMenu;
    QAction* mBreakpointHardwareWrite1;
    QAction* mBreakpointHardwareWrite2;
    QAction* mBreakpointHardwareWrite4;
#ifdef _WIN64
    QAction* mBreakpointHardwareWrite8;
#endif //_WIN64
    QAction* mBreakpointHardwareRemove;
    QMenu* mBreakpointMemoryAccessMenu;
    QMenu* mBreakpointMemoryWriteMenu;
    QAction* mBreakpointMemoryAccessSingleshoot;
    QAction* mBreakpointMemoryAccessRestore;
    QAction* mBreakpointMemoryWriteSingleShoot;
    QAction* mBreakpointMemoryWriteRestore;
    QAction* mBreakpointMemoryRemove;
    QAction* mModifyAction;
    QAction* mUndoSelection;
    QAction* mGotoSp;
    QAction* mGotoBp;
    QAction* mFreezeStack;
    QAction* mGotoExpression;
    QAction* mGotoPrevious;
    QAction* mGotoNextFrame;
    QAction* mGotoPrevFrame;
    QAction* mGotoFrameBase;
    QAction* mGotoNext;
    QAction* mFindPatternAction;
    QAction* mFollowDisasm;
    QAction* mFollowPtrDump;
    QAction* mFollowInDump;
    QAction* mFollowInMemoryMap;
    QAction* mFollowStack;
    QAction* mWatchData;
    QMenu* mPluginMenu;
    QMenu* mFollowInDumpMenu;
    QAction* mPushAction;
    QAction* mPopAction;
    QAction* mRealignAction;
    QList<QAction*> mFollowInDumpActions;

    GotoDialog* mGoto;
    CPUMultiDump* mMultiDump;
    QColor mUserStackFrameColor;
    QColor mSystemStackFrameColor;
    QColor mStackReturnToColor;
    QColor mStackSEHChainColor;
    struct CPUCallStack
    {
        duint addr;
        int party;
    };

    std::vector<CPUCallStack> mCallstack;
    static int CPUStack::getCurrentFrame(const std::vector<CPUStack::CPUCallStack> & mCallstack, duint wVA);
};

#endif // CPUSTACK_H

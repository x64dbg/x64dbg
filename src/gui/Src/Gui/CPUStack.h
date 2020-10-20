#ifndef CPUSTACK_H
#define CPUSTACK_H

#include "HexDump.h"

//forward declaration
class CPUMultiDump;
class GotoDialog;
class CommonActions;

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
    void stackDumpAt(duint addr, duint csp);
    void gotoCspSlot();
    void gotoCbpSlot();
    void gotoExpressionSlot();
    void gotoPreviousFrameSlot();
    void gotoNextFrameSlot();
    void gotoFrameBaseSlot();
    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);
    void selectionUpdatedSlot();
    void followDisasmSlot();
    void followinDumpNSlot();
    void followStackSlot();
    void binaryEditSlot();
    void binaryFillSlot();
    void binaryCopySlot();
    void binaryPasteSlot();
    void findPattern();
    void binaryPasteIgnoreSizeSlot();
    void undoSelectionSlot();
    void modifySlot();
    void realignSlot();
    void freezeStackSlot();
    void dbgStateChangedSlot(DBGSTATE state);
    void updateSlot();

private:
    duint mCsp;
    bool bStackFrozen;

    QAction* mFreezeStack;
    QAction* mFollowStack;
    QAction* mFollowDisasm;
    QList<QAction*> mFollowInDumpActions;
    QMenu* mPluginMenu;

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

    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;

    std::vector<CPUCallStack> mCallstack;
    static int CPUStack::getCurrentFrame(const std::vector<CPUStack::CPUCallStack> & mCallstack, duint wVA);
};

#endif // CPUSTACK_H

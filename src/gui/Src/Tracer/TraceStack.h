#pragma once

#include "HexDump.h"

//forward declaration
class CPUMultiDump;
class GotoDialog;
class CommonActions;
class TraceWidget;
class TraceBrowser;
class TraceFileDumpMemoryPage;

class TraceStack : public HexDump
{
    Q_OBJECT
public:
    explicit TraceStack(Architecture* architecture, TraceWidget* parent, TraceFileDumpMemoryPage* memoryPage);

    // Configuration
    void updateColors() override;
    void updateFonts() override;

    void getColumnRichText(duint col, duint rva, RichTextPainter::List & richText) override;
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void setupContextMenu();
    //void updateFreezeStackAction();
    void printDumpAt(duint parVA, bool select, bool repaint, bool updateTableOffset) override;
    void reloadData() override;

signals:
    void displayReferencesWidget();
    void xrefSignal(duint addr);

public slots:
    void stackDumpAt(duint addr, duint csp);
    void gotoCspSlot();
    void gotoCbpSlot();
    void gotoExpressionSlot();
    void gotoXrefSlot();
    void selectionUpdatedSlot();
    void followDisasmSlot();
    void followStackSlot();
    void binaryCopySlot();
    //void freezeStackSlot();
    //void disasmSelectionChanged(duint parVA);
    void updateSlot();
    void copyPtrColumnSlot();
    void copyCommentsColumnSlot();

private:
    TraceWidget* mParent;

    duint mCsp = 0;
    bool bStackFrozen = false;

    QAction* mFreezeStack;
    QAction* mFollowStack;
    QAction* mFollowDisasm;

    GotoDialog* mGoto;
    //CPUMultiDump* mMultiDump;
    //QColor mUserStackFrameColor;
    //QColor mSystemStackFrameColor;
    //QColor mStackReturnToColor;
    //QColor mStackSEHChainColor;
    //struct CPUCallStack
    //{
    //    duint addr;
    //    int party;
    //};

    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;

    //std::vector<CPUCallStack> mCallstack;
    //static int CPUStack::getCurrentFrame(const std::vector<CPUStack::CPUCallStack> & mCallstack, duint va);
};

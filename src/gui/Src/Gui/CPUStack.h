#ifndef CPUSTACK_H
#define CPUSTACK_H

#include "HexDump.h"
#include "GotoDialog.h"

//forward declaration
class CPUMultiDump;

class CPUStack : public HexDump
{
    Q_OBJECT
public:
    explicit CPUStack(CPUMultiDump* multiDump, QWidget* parent = 0);

    // Configuration
    virtual void updateColors();
    virtual void updateFonts();

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void contextMenuEvent(QContextMenuEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void setupContextMenu();
    void updateFreezeStackAction();

signals:
    void displayReferencesWidget();

public slots:
    void refreshShortcutsSlot();
    void stackDumpAt(duint addr, duint csp);
    void gotoSpSlot();
    void gotoBpSlot();
    void gotoExpressionSlot();
    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);
    void selectionUpdatedSlot();
    void followDisasmSlot();
    void followDumpSlot();
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
    void freezeStackSlot();
    void dbgStateChangedSlot(DBGSTATE state);

private:
    duint mCsp;
    bool bStackFrozen;

    QMenu* mBinaryMenu;
    QAction* mBinaryEditAction;
    QAction* mBinaryFillAction;
    QAction* mBinaryCopyAction;
    QAction* mBinaryPasteAction;
    QAction* mBinaryPasteIgnoreSizeAction;
    QAction* mModifyAction;
    QAction* mUndoSelection;
    QAction* mGotoSp;
    QAction* mGotoBp;
    QAction* mFreezeStack;
    QAction* mGotoExpression;
    QAction* mFindPatternAction;
    QAction* mFollowDisasm;
    QAction* mFollowDump;
    QAction* mFollowStack;
    QMenu* mPluginMenu;
    QMenu* mFollowInDumpMenu;
    QList<QAction*> mFollowInDumpActions;

    GotoDialog* mGoto;
    CPUMultiDump* mMultiDump;
};

#endif // CPUSTACK_H

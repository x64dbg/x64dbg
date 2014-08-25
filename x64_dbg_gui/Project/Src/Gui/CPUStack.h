#ifndef CPUSTACK_H
#define CPUSTACK_H

#include "HexDump.h"
#include "GotoDialog.h"

class CPUStack : public HexDump
{
    Q_OBJECT
public:
    explicit CPUStack(QWidget* parent = 0);
    void colorsUpdated();
    void fontsUpdated();
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void contextMenuEvent(QContextMenuEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void setupContextMenu();

signals:
    void displayReferencesWidget();

public slots:
    void refreshShortcutsSlot();
    void stackDumpAt(uint_t addr, uint_t csp);
    void gotoSpSlot();
    void gotoBpSlot();
    void gotoExpressionSlot();
    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);
    void followDisasmSlot();
    void followDumpSlot();
    void followStackSlot();
    void binaryEditSlot();
    void binaryFillSlot();
    void binaryCopySlot();
    void binaryPasteSlot();
    void findPattern();
    void binaryPasteIgnoreSizeSlot();
    void undoSelectionSlot();
    void modifySlot();

private:
    uint_t mCsp;

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
    QAction* mGotoExpression;
    QAction* mFindPatternAction;
    QAction* mFollowDisasm;
    QAction* mFollowDump;
    QAction* mFollowStack;

    GotoDialog* mGoto;
};

#endif // CPUSTACK_H

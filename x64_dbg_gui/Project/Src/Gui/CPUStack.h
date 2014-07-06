#ifndef CPUSTACK_H
#define CPUSTACK_H

#include <QtGui>
#include <QtDebug>
#include <QAction>
#include <QMenu>
#include "NewTypes.h"
#include "HexDump.h"
#include "Bridge.h"
#include "GotoDialog.h"

class CPUStack : public HexDump
{
    Q_OBJECT
public:
    explicit CPUStack(QWidget *parent = 0);
    void colorsUpdated();
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void contextMenuEvent(QContextMenuEvent* event);

    void setupContextMenu();

signals:
    void displayReferencesWidget();

public slots:
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

private:
    uint_t mCsp;

    QMenu* mBinaryMenu;
    QAction* mBinaryEditAction;
    QAction* mBinaryFillAction;
    QAction* mBinaryCopyAction;
    QAction* mBinaryPasteAction;
    QAction* mBinaryPasteIgnoreSizeAction;
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

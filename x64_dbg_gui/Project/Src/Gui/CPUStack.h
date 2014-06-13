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

private:
    uint_t mCsp;

    QAction* mGotoSp;
    QAction* mGotoBp;
    QAction* mGotoExpression;
    QAction* mFollowDisasm;
    QAction* mFollowDump;
    QAction* mFollowStack;

    GotoDialog* mGoto;
};

#endif // CPUSTACK_H

#pragma once

#include "StdTable.h"
#include <QMenu>

class ThreadView : public StdTable
{
    Q_OBJECT
public:
    explicit ThreadView(StdTable* parent = nullptr);
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    void setupContextMenu();

signals:
    void displayThreadsView();

public slots:
    void selectionThreadsSet(const SELECTIONDATA* selection);
    void selectionThreadsGet(SELECTIONDATA* selection);
    void updateThreadListSlot();
    void doubleClickedSlot();
    void execCommandSlot();
    void gotoThreadEntrySlot();
    void contextMenuSlot(const QPoint & pos);
    void setNameSlot();

private:
    QAction* makeCommandAction(QAction* action, const QString & command);
    duint mCurrentThreadId;
    MenuBuilder* mMenuBuilder;

    enum
    {
        ColNumber = 0,
        ColThreadId,
        ColEntry,
        ColTeb,
        ColCip,
        ColSuspendCount,
        ColPriority,
        ColWaitReason,
        ColLastError,
        ColUserTime,
        ColKernelTime,
        ColCreationTime,
        ColCpuCycles,
        ColThreadName,
    };
};

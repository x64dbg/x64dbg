#ifndef THREADVIEW_H
#define THREADVIEW_H

#include "StdTable.h"
#include <QMenu>

class ThreadView : public StdTable
{
    Q_OBJECT
public:
    explicit ThreadView(StdTable* parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();

public slots:
    void updateThreadList();
    void doubleClickedSlot();
    void SwitchThread();
    void SuspendThread();
    void ResumeThread();
    void KillThread();
    void GoToThreadEntry();
    void contextMenuSlot(const QPoint & pos);
    void SetPriorityIdleSlot();
    void SetPriorityAboveNormalSlot();
    void SetPriorityBelowNormalSlot();
    void SetPriorityHighestSlot();
    void SetPriorityLowestSlot();
    void SetPriorityNormalSlot();
    void SetPriorityTimeCriticalSlot();

signals:
    void showCpu();

private:
    int mCurrentThread;
    QAction* mSwitchThread;
    QAction* mSuspendThread;
    QAction* mGoToThreadEntry;
    QAction* mResumeThread;
    QAction* mKillThread;
    QAction* mSetPriorityIdle;
    QAction* mSetPriorityAboveNormal;
    QAction* mSetPriorityBelowNormal;
    QAction* mSetPriorityHighest;
    QAction* mSetPriorityLowest;
    QAction* mSetPriorityNormal;
    QAction* mSetPriorityTimeCritical;
    QMenu* mSetPriority;
};

#endif // THREADVIEW_H

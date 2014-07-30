#ifndef THREADVIEW_H
#define THREADVIEW_H

#include "StdTable.h"
#include "Bridge.h"

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
    void SuspendThread();
    void ResumeThread();
    void KillThread();
    void contextMenuSlot(const QPoint &pos);
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
    QAction* mSuspendThread;
    QAction* mResumeThread;
    QAction* mKillThread;
    QAction * mSetPriorityIdle;
    QAction * mSetPriorityAboveNormal;
    QAction * mSetPriorityBelowNormal;
    QAction * mSetPriorityHighest;
    QAction * mSetPriorityLowest;
    QAction * mSetPriorityNormal;
    QAction * mSetPriorityTimeCritical;
    QMenu* mSetPriority;
};

#endif // THREADVIEW_H

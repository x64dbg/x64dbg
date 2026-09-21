#pragma once

#include <BasicView/StdTable.h>

#include "core/DbgAdapter.h"

class QMenu;
class QAction;

class ThreadView : public StdTable
{
    Q_OBJECT
public:
    explicit ThreadView(DbgAdapter* adapter, QWidget* parent = nullptr);

    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;

public slots:
    void onThreadsUpdated(const QVector<DbgThreadInfo> & threads, pid_t currentTid);
    void onSessionEnded();

private slots:
    void onSwitchThread();
    void onSetName();
    void onSuspendThread();
    void onResumeThread();
    void onSuspendAll();
    void onResumeAll();
    void onContextMenu(const QPoint & pos) const;

private:
    enum
    {
        ColNumber = 0,
        ColThreadId,
        ColFsBase,
        ColRip,
        ColSuspendCount,
        ColPriority,
        ColWaitReason,
        ColUserTime,
        ColKernelTime,
        ColCreationTime,
        ColName,
    };

    void setupContextMenu();
    void setSelectedSuspended(bool suspended);

    DbgAdapter* mAdapter = nullptr;
    pid_t mCurrentThreadId = 0;
    QMenu* mContextMenu = nullptr;
    QAction* mSwitchAction = nullptr;
    QAction* mSetNameAction = nullptr;
    QAction* mSuspendAction = nullptr;
    QAction* mResumeAction = nullptr;
    QAction* mSuspendAllAction = nullptr;
    QAction* mResumeAllAction = nullptr;
};

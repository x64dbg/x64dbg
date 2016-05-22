#ifndef BREAKPOINTSVIEW_H
#define BREAKPOINTSVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include "StdTable.h"

class BreakpointsView : public QWidget
{
    Q_OBJECT
public:
    explicit BreakpointsView(QWidget* parent = 0);
    void setupHardBPRightClickContextMenu();
    void setupSoftBPRightClickContextMenu();
    void setupMemBPRightClickContextMenu();
    void setupCondBPRightClickContextMenu();

signals:
    void showCpu();

public slots:
    void refreshShortcutsSlot();
    void reloadData();

    // Hardware
    void hardwareBPContextMenuSlot(const QPoint & pos);
    void removeHardBPActionSlot();
    void removeAllHardBPActionSlot();
    void enableDisableHardBPActionSlot();
    void doubleClickHardwareSlot();
    void resetHardwareHitCountSlot();

    // Software
    void softwareBPContextMenuSlot(const QPoint & pos);
    void removeSoftBPActionSlot();
    void removeAllSoftBPActionSlot();
    void enableDisableSoftBPActionSlot();
    void enableAllSoftBPActionSlot();
    void disableAllSoftBPActionSlot();
    void doubleClickSoftwareSlot();
    void resetSoftwareHitCountSlot();

    // Memory
    void memoryBPContextMenuSlot(const QPoint & pos);
    void removeMemBPActionSlot();
    void removeAllMemBPActionSlot();
    void enableDisableMemBPActionSlot();
    void doubleClickMemorySlot();
    void resetMemoryHitCountSlot();

    // Conditional
    void setLogSlot();
    void setCmdSlot();
    void setFastResumeSlot();
    void setConditionSlot();

private:
    QVBoxLayout* mVertLayout;
    QSplitter* mSplitter ;
    StdTable* mHardBPTable;
    StdTable* mSoftBPTable;
    StdTable* mMemBPTable;
    QMenu* mConditionalBreakpointMenu;
    // Conditional BP Context Menu
    int CurrentType;
    QAction* mConditionalSetCondition;
    QAction* mConditionalSetFastResume;
    QAction* mConditionalSetLog;
    QAction* mConditionalSetCmd;

    // Hardware BP Context Menu
    QAction* mHardBPRemoveAction;
    QAction* mHardBPRemoveAllAction;
    QAction* mHardBPEnableDisableAction;
    QAction* mHardBPResetHitCountAction;

    // Software BP Context Menu
    QAction* mSoftBPRemoveAction;
    QAction* mSoftBPRemoveAllAction;
    QAction* mSoftBPEnableDisableAction;
    QAction* mSoftBPEnableAllAction;
    QAction* mSoftBPDisableAllAction;
    QAction* mSoftBPResetHitCountAction;

    // Memory BP Context Menu
    QAction* mMemBPRemoveAction;
    QAction* mMemBPRemoveAllAction;
    QAction* mMemBPEnableDisableAction;
    QAction* mMemBPResetHitCountAction;
};

#endif // BREAKPOINTSVIEW_H

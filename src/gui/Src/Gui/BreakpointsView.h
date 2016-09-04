#ifndef BREAKPOINTSVIEW_H
#define BREAKPOINTSVIEW_H

#include <QWidget>
#include "Imports.h"

class StdTable;
class QVBoxLayout;
class LabeledSplitter;

class BreakpointsView : public QWidget
{
    Q_OBJECT
public:
    explicit BreakpointsView(QWidget* parent = 0);
    void setupRightClickContextMenu();
    void setupHardBPRightClickContextMenu();
    void setupSoftBPRightClickContextMenu();
    void setupMemBPRightClickContextMenu();
    void setupDLLBPRightClickContextMenu();

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
    void enableAllHardBPActionSlot();
    void disableAllHardBPActionSlot();
    void doubleClickHardwareSlot();
    void selectionChangedHardwareSlot();
    void resetHardwareHitCountSlot();

    // Software
    void softwareBPContextMenuSlot(const QPoint & pos);
    void removeSoftBPActionSlot();
    void removeAllSoftBPActionSlot();
    void enableDisableSoftBPActionSlot();
    void enableAllSoftBPActionSlot();
    void disableAllSoftBPActionSlot();
    void doubleClickSoftwareSlot();
    void selectionChangedSoftwareSlot();
    void resetSoftwareHitCountSlot();

    // Memory
    void memoryBPContextMenuSlot(const QPoint & pos);
    void removeMemBPActionSlot();
    void removeAllMemBPActionSlot();
    void enableDisableMemBPActionSlot();
    void enableAllMemBPActionSlot();
    void disableAllMemBPActionSlot();
    void doubleClickMemorySlot();
    void selectionChangedMemorySlot();
    void resetMemoryHitCountSlot();

    // DLL
    void DLLBPContextMenuSlot(const QPoint & pos);
    void removeDLLBPActionSlot();
    void enableDisableDLLBPActionSlot();
    void selectionChangedDLLSlot();
    void resetDLLHitCountSlot();

    // Conditional
    void editBreakpointSlot();

private:
    QVBoxLayout* mVertLayout;
    LabeledSplitter* mSplitter;
    StdTable* mHardBPTable;
    StdTable* mSoftBPTable;
    StdTable* mMemBPTable;
    StdTable* mDLLBPTable;
    // Conditional BP Context Menu
    BPXTYPE mCurrentType;
    QAction* mEditBreakpointAction;

    // Hardware BP Context Menu
    QAction* mHardBPRemoveAction;
    QAction* mHardBPRemoveAllAction;
    QAction* mHardBPEnableDisableAction;
    QAction* mHardBPResetHitCountAction;
    QAction* mHardBPEnableAllAction;
    QAction* mHardBPDisableAllAction;

    // Software BP Context Menu
    QAction* mSoftBPRemoveAction;
    QAction* mSoftBPRemoveAllAction;
    QAction* mSoftBPEnableDisableAction;
    QAction* mSoftBPResetHitCountAction;
    QAction* mSoftBPEnableAllAction;
    QAction* mSoftBPDisableAllAction;

    // Memory BP Context Menu
    QAction* mMemBPRemoveAction;
    QAction* mMemBPRemoveAllAction;
    QAction* mMemBPEnableDisableAction;
    QAction* mMemBPResetHitCountAction;
    QAction* mMemBPEnableAllAction;
    QAction* mMemBPDisableAllAction;

    // DLL BP Context Menu
    QAction* mDLLBPRemoveAction;
    QAction* mDLLBPEnableDisableAction;
    QAction* mDLLBPResetHitCountAction;
};

#endif // BREAKPOINTSVIEW_H

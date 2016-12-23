#ifndef BREAKPOINTSVIEW_H
#define BREAKPOINTSVIEW_H

#include <QWidget>
#include "Imports.h"

class StdTable;
class QVBoxLayout;
class LabeledSplitter;
class MenuBuilder;
class BreakpointsViewTable;

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
    void setupExceptionBPRightClickContextMenu();

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
    void addDLLBPActionSlot();
    void removeDLLBPActionSlot();
    void removeAllDLLBPActionSlot();
    void enableDisableDLLBPActionSlot();
    void enableAllDLLBPActionSlot();
    void disableAllDLLBPActionSlot();
    void selectionChangedDLLSlot();
    void resetDLLHitCountSlot();

    // Exception
    void ExceptionBPContextMenuSlot(const QPoint & pos);
    void addExceptionBPActionSlot();
    void removeExceptionBPActionSlot();
    void removeAllExceptionBPActionSlot();
    void enableDisableExceptionBPActionSlot();
    void enableAllExceptionBPActionSlot();
    void disableAllExceptionBPActionSlot();
    void selectionChangedExceptionSlot();
    void resetExceptionHitCountSlot();

    // Conditional
    void editBreakpointSlot();

private:
    QVBoxLayout* mVertLayout;
    LabeledSplitter* mSplitter;
    StdTable* mHardBPTable;
    StdTable* mSoftBPTable;
    StdTable* mMemBPTable;
    StdTable* mDLLBPTable;
    StdTable* mExceptionBPTable;
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
    QAction* mDLLBPAddAction;
    QAction* mDLLBPRemoveAction;
    QAction* mDLLBPRemoveAllAction;
    QAction* mDLLBPEnableDisableAction;
    QAction* mDLLBPResetHitCountAction;
    QAction* mDLLBPEnableAllAction;
    QAction* mDLLBPDisableAllAction;

    // Exception BP Context Menu
    QAction* mExceptionBPAddAction;
    QAction* mExceptionBPRemoveAction;
    QAction* mExceptionBPRemoveAllAction;
    QAction* mExceptionBPEnableDisableAction;
    QAction* mExceptionBPResetHitCountAction;
    QAction* mExceptionBPEnableAllAction;
    QAction* mExceptionBPDisableAllAction;
};

#endif // BREAKPOINTSVIEW_H

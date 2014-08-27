#ifndef MEMORYMAPVIEW_H
#define MEMORYMAPVIEW_H

#include "StdTable.h"

class MemoryMapView : public StdTable
{
    Q_OBJECT
public:
    explicit MemoryMapView(StdTable* parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();

signals:
    void showCpu();

public slots:
    void refreshShortcutsSlot();
    void stateChangedSlot(DBGSTATE state);
    void followDumpSlot();
    void followDisassemblerSlot();
    void memoryAccessSingleshootSlot();
    void memoryAccessRestoreSlot();
    void memoryWriteSingleshootSlot();
    void memoryWriteRestoreSlot();
    void memoryExecuteSingleshootSlot();
    void memoryExecuteRestoreSlot();
    void memoryRemoveSlot();
    void memoryExecuteSingleshootToggleSlot();
    void contextMenuSlot(const QPoint & pos);
    void switchView();
    void pageMemoryRights();
    void refreshMap();

private:
    QString getProtectionString(DWORD Protect);

    QAction* mFollowDump;
    QAction* mFollowDisassembly;
    QAction* mSwitchView;
    QAction* mPageMemoryRights;

    QMenu* mBreakpointMenu;
    QMenu* mMemoryAccessMenu;
    QAction* mMemoryAccessSingleshoot;
    QAction* mMemoryAccessRestore;
    QMenu* mMemoryWriteMenu;
    QAction* mMemoryWriteSingleshoot;
    QAction* mMemoryWriteRestore;
    QMenu* mMemoryExecuteMenu;
    QAction* mMemoryExecuteSingleshoot;
    QAction* mMemoryExecuteRestore;
    QAction* mMemoryRemove;
    QAction* mMemoryExecuteSingleshootToggle;

};

#endif // MEMORYMAPVIEW_H

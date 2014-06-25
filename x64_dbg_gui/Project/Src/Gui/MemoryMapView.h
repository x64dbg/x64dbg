#ifndef MEMORYMAPVIEW_H
#define MEMORYMAPVIEW_H

#include <QtGui>
#include "StdTable.h"
#include "Bridge.h"

class MemoryMapView : public StdTable
{
    Q_OBJECT
public:
    explicit MemoryMapView(StdTable *parent = 0);
    QString paintContent(QPainter *painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();
    void contextMenuEvent(QContextMenuEvent* event);
    
signals:
    
public slots:
    void stateChangedSlot(DBGSTATE state);
    void memoryAccessSingleshootSlot();
    void memoryAccessRestoreSlot();
    void memoryWriteSingleshootSlot();
    void memoryWriteRestoreSlot();
    void memoryExecuteSingleshootSlot();
    void memoryExecuteRestoreSlot();
    void memoryRemoveSlot();
    void memoryExecuteSingleshootToggleSlot();

private:
    QString getProtectionString(DWORD Protect);

    QAction* mFollowDump;
    QAction* mFollowDisassembly;

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

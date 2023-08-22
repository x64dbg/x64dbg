#pragma once

#include "StdIconTable.h"

class GotoDialog;

class MemoryMapView : public StdIconTable
{
    Q_OBJECT
public:
    explicit MemoryMapView(StdTable* parent = 0);
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();

signals:
    void showReferences();

public slots:
    void refreshShortcutsSlot();
    void stateChangedSlot(DBGSTATE state);
    void followDumpSlot();
    void followDisassemblerSlot();
    void followSymbolsSlot();
    void doubleClickedSlot();
    void memoryExecuteSingleshootToggleSlot();
    void memoryAllocateSlot();
    void ExecCommand();
    void contextMenuSlot(const QPoint & pos);
    void switchView();
    void pageMemoryRights();
    void refreshMap();
    void findPatternSlot();
    void dumpMemory();
    void loadMemory();
    void commentSlot();
    void selectAddress(duint va);
    void gotoOriginSlot();
    void gotoExpressionSlot();
    void addVirtualModSlot();
    void findReferencesSlot();
    void selectionGetSlot(SELECTIONDATA* selection);
    void disassembleAtSlot(dsint va, dsint cip);

private:
    void setSwitchViewName();

    enum
    {
        ColAddress = 0,
        ColSize,
        ColParty,
        ColPageInfo,
        ColContent,
        ColAllocation,
        ColCurProtect,
        ColAllocProtect
    };

    inline duint getSelectionAddr()
    {
        return getCellUserdata(getInitialSelection(), ColAddress);
    }

    inline QString getSelectionText()
    {
        return getCellContent(getInitialSelection(), ColAddress);
    }

    QAction* makeCommandAction(QAction* action, const QString & command);

    GotoDialog* mGoto = nullptr;

    QAction* mFollowDump;
    QAction* mFollowDisassembly;
    QAction* mFollowSymbols;
    QAction* mSwitchView;
    QAction* mPageMemoryRights;
    QAction* mDumpMemory;
    QAction* mLoadMemory;

    QMenu* mBreakpointMenu;
    QMenu* mMemoryAccessMenu;
    QAction* mMemoryAccessSingleshoot;
    QAction* mMemoryAccessRestore;
    QMenu* mMemoryReadMenu;
    QAction* mMemoryReadSingleshoot;
    QAction* mMemoryReadRestore;
    QMenu* mMemoryWriteMenu;
    QAction* mMemoryWriteSingleshoot;
    QAction* mMemoryWriteRestore;
    QMenu* mMemoryExecuteMenu;
    QAction* mMemoryExecuteSingleshoot;
    QAction* mMemoryExecuteRestore;
    QAction* mMemoryRemove;
    QAction* mMemoryExecuteSingleshootToggle;
    QAction* mFindPattern;
    QMenu* mGotoMenu;
    QAction* mGotoOrigin;
    QAction* mGotoExpression;
    QAction* mMemoryAllocate;
    QAction* mMemoryFree;
    QAction* mAddVirtualMod;
    QAction* mComment;
    QAction* mReferences;
    QMenu* mPluginMenu;

    duint mCipBase;
};

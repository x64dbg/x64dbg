#pragma once

#include "StdIconTable.h"

class GotoDialog;

class MemoryMapView : public StdIconTable
{
    Q_OBJECT
public:
    explicit MemoryMapView(StdTable* parent = nullptr);
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    void setupContextMenu();

signals:
    void showReferences();

public slots:
    void selectionChangedSlot(duint index);
    void fixSelectionRangeSlot();
    void refreshShortcutsSlot();
    void stateChangedSlot(DBGSTATE state);
    void followDumpSlot();
    void followDisassemblerSlot();
    void followSymbolsSlot();
    void doubleClickedSlot();
    void memoryExecuteSingleshootToggleSlot();
    void memoryAllocateSlot();
    void execCommandSlot();
    void contextMenuSlot(const QPoint & pos);
    void switchViewSlot();
    void pageMemoryRightsSlot();
    void refreshMapSlot();
    void findPatternSlot();
    void dumpMemorySlot();
    void loadMemorySlot();
    void commentSlot();
    void selectAddressSlot(duint va);
    void gotoOriginSlot();
    void gotoExpressionSlot();
    void addVirtualModSlot();
    void findReferencesSlot();
    void selectionGetSlot(SELECTIONDATA* selection);
    void selectionSetSlot(const SELECTIONDATA* selection);
    void disassembleAtSlot(duint va, duint cip);

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

    duint mCipBase = 0;
    duint mSelectionStart = 0;
    duint mSelectionEnd = 0;
    duint mSelectionCount = 0;
    duint mSelectionSort = -1;
};

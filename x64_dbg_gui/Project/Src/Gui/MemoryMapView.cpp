#include "MemoryMapView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "PageMemoryRights.h"

MemoryMapView::MemoryMapView(StdTable* parent) : StdTable(parent)
{
    enableMultiSelection(false);

    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * 2 * sizeof(uint_t), "ADDR", false, "Address"); //addr
    addColumnAt(8 + charwidth * 2 * sizeof(uint_t), "SIZE", false, "Size"); //size
    addColumnAt(8 + charwidth * 32, "INFO", false, "Page Information"); //page information
    addColumnAt(8 + charwidth * 3, "TYP", false, "Allocation Type"); //allocation type
    addColumnAt(8 + charwidth * 5, "CPROT", false, "Current Protection"); //current protection
    addColumnAt(8 + charwidth * 5, "APROT", false, "Allocation Protection"); //allocation protection
    addColumnAt(100, "", false);

    connect(Bridge::getBridge(), SIGNAL(updateMemory()), this, SLOT(refreshMap()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(stateChangedSlot(DBGSTATE)));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    setupContextMenu();
}

void MemoryMapView::setupContextMenu()
{
    //Follow in Dump
    mFollowDump = new QAction("&Follow in Dump", this);
    connect(mFollowDump, SIGNAL(triggered()), this, SLOT(followDumpSlot()));

    //Follow in Disassembler
    mFollowDisassembly = new QAction("Follow in &Disassembler", this);
    mFollowDisassembly->setShortcutContext(Qt::WidgetShortcut);
    mFollowDisassembly->setShortcut(QKeySequence("enter"));
    connect(mFollowDisassembly, SIGNAL(triggered()), this, SLOT(followDisassemblerSlot()));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followDisassemblerSlot()));

    //Set PageMemory Rights
    mPageMemoryRights = new QAction("Set Page Memory Rights", this);
    connect(mPageMemoryRights, SIGNAL(triggered()), this, SLOT(pageMemoryRights()));

    //Switch View
    mSwitchView = new QAction("&Switch View", this);
    connect(mSwitchView, SIGNAL(triggered()), this, SLOT(switchView()));

    //Breakpoint menu
    mBreakpointMenu = new QMenu("Memory &Breakpoint", this);

    //Breakpoint->Memory Access
    mMemoryAccessMenu = new QMenu("Access", this);
    mMemoryAccessSingleshoot = new QAction("&Singleshoot", this);
    connect(mMemoryAccessSingleshoot, SIGNAL(triggered()), this, SLOT(memoryAccessSingleshootSlot()));
    mMemoryAccessMenu->addAction(mMemoryAccessSingleshoot);
    mMemoryAccessRestore = new QAction("&Restore", this);
    connect(mMemoryAccessRestore, SIGNAL(triggered()), this, SLOT(memoryAccessRestoreSlot()));
    mMemoryAccessMenu->addAction(mMemoryAccessRestore);
    mBreakpointMenu->addMenu(mMemoryAccessMenu);

    //Breakpoint->Memory Write
    mMemoryWriteMenu = new QMenu("Write", this);
    mMemoryWriteSingleshoot = new QAction("&Singleshoot", this);
    connect(mMemoryWriteSingleshoot, SIGNAL(triggered()), this, SLOT(memoryWriteSingleshootSlot()));
    mMemoryWriteMenu->addAction(mMemoryWriteSingleshoot);
    mMemoryWriteRestore = new QAction("&Restore", this);
    connect(mMemoryWriteRestore, SIGNAL(triggered()), this, SLOT(memoryWriteRestoreSlot()));
    mMemoryWriteMenu->addAction(mMemoryWriteRestore);
    mBreakpointMenu->addMenu(mMemoryWriteMenu);

    //Breakpoint->Memory Execute
    mMemoryExecuteMenu = new QMenu("Execute", this);
    mMemoryExecuteSingleshoot = new QAction("&Singleshoot", this);
    mMemoryExecuteSingleshoot->setShortcutContext(Qt::WidgetShortcut);
    connect(mMemoryExecuteSingleshoot, SIGNAL(triggered()), this, SLOT(memoryExecuteSingleshootSlot()));
    mMemoryExecuteMenu->addAction(mMemoryExecuteSingleshoot);
    mMemoryExecuteRestore = new QAction("&Restore", this);
    connect(mMemoryExecuteRestore, SIGNAL(triggered()), this, SLOT(memoryExecuteRestoreSlot()));
    mMemoryExecuteMenu->addAction(mMemoryExecuteRestore);
    mBreakpointMenu->addMenu(mMemoryExecuteMenu);

    //Breakpoint->Remove
    mMemoryRemove = new QAction("&Remove", this);
    mMemoryRemove->setShortcutContext(Qt::WidgetShortcut);
    connect(mMemoryRemove, SIGNAL(triggered()), this, SLOT(memoryRemoveSlot()));
    mBreakpointMenu->addAction(mMemoryRemove);

    //Action shortcut action that does something
    mMemoryExecuteSingleshootToggle = new QAction(this);
    mMemoryExecuteSingleshootToggle->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mMemoryExecuteSingleshootToggle);
    connect(mMemoryExecuteSingleshootToggle, SIGNAL(triggered()), this, SLOT(memoryExecuteSingleshootToggleSlot()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void MemoryMapView::refreshShortcutsSlot()
{
    mMemoryExecuteSingleshoot->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mMemoryRemove->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mMemoryExecuteSingleshootToggle->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
}

void MemoryMapView::contextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;
    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addAction(mFollowDisassembly);
    wMenu->addAction(mFollowDump);
    wMenu->addAction(mSwitchView);
    wMenu->addSeparator();
    wMenu->addAction(mPageMemoryRights);
    wMenu->addSeparator();
    wMenu->addMenu(mBreakpointMenu);
    QMenu wCopyMenu("&Copy", this);
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
    }

    QString wStr = getCellContent(getInitialSelection(), 0);
#ifdef _WIN64
    uint_t selectedAddr = wStr.toULongLong(0, 16);
#else //x86
    uint_t selectedAddr = wStr.toULong(0, 16);
#endif //_WIN64
    if((DbgGetBpxTypeAt(selectedAddr)&bp_memory) == bp_memory) //memory breakpoint set
    {
        mMemoryAccessMenu->menuAction()->setVisible(false);
        mMemoryWriteMenu->menuAction()->setVisible(false);
        mMemoryExecuteMenu->menuAction()->setVisible(false);
        mMemoryRemove->setVisible(true);
    }
    else //memory breakpoint not set
    {
        mMemoryAccessMenu->menuAction()->setVisible(true);
        mMemoryWriteMenu->menuAction()->setVisible(true);
        mMemoryExecuteMenu->menuAction()->setVisible(true);
        mMemoryRemove->setVisible(false);
    }

    wMenu->exec(mapToGlobal(pos)); //execute context menu
}

QString MemoryMapView::getProtectionString(DWORD Protect)
{
#define RIGHTS_STRING (sizeof("ERWCG") + 1)
    char rights[RIGHTS_STRING];

    if(!DbgFunctions()->PageRightsToString(Protect, rights))
        return "bad";

    return QString(rights);
}

QString MemoryMapView::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if(col == 0) //address
    {
        QString wStr = getCellContent(rowBase + rowOffset, col);
#ifdef _WIN64
        uint_t addr = wStr.toULongLong(0, 16);
#else //x86
        uint_t addr = wStr.toULong(0, 16);
#endif //_WIN64
        if((DbgGetBpxTypeAt(addr)&bp_memory) == bp_memory)
        {
            QString wStr = getCellContent(rowBase + rowOffset, col);
            QColor bpBackgroundColor = ConfigColor("MemoryMapBreakpointBackgroundColor");
            if(bpBackgroundColor.alpha())
                painter->fillRect(QRect(x, y, w - 1, h), QBrush(bpBackgroundColor));
            painter->setPen(ConfigColor("MemoryMapBreakpointColor"));
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
            return "";
        }
        else if(isSelected(rowBase, rowOffset) == true)
            painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor));
    }
    else if(col == 2) //info
    {
        QString wStr = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);;
        if(wStr.startsWith(" \""))
        {
            painter->setPen(ConfigColor("MemoryMapSectionTextColor"));
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
            return "";
        }
    }
    else if(col == 4) //CPROT
    {
        duint setting = 0;
        QString wStr = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);;
        if(BridgeSettingGetUint("Engine", "ListAllPages", &setting) && !setting)
        {
            painter->setPen(ConfigColor("MemoryMapSectionTextColor"));
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
            return "";
        }
    }
    return StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
}

void MemoryMapView::refreshMap()
{
    MEMMAP wMemMapStruct;
    int wI;

    memset(&wMemMapStruct, 0, sizeof(MEMMAP));

    DbgMemMap(&wMemMapStruct);

    setRowCount(wMemMapStruct.count);

    for(wI = 0; wI < wMemMapStruct.count; wI++)
    {
        QString wS;
        MEMORY_BASIC_INFORMATION wMbi = (wMemMapStruct.page)[wI].mbi;

        // Base address
        wS = QString("%1").arg((uint_t)wMbi.BaseAddress, sizeof(uint_t) * 2, 16, QChar('0')).toUpper();
        setCellContent(wI, 0, wS);

        // Size
        wS = QString("%1").arg((uint_t)wMbi.RegionSize, sizeof(uint_t) * 2, 16, QChar('0')).toUpper();
        setCellContent(wI, 1, wS);

        // Information
        wS = QString((wMemMapStruct.page)[wI].info);
        setCellContent(wI, 2, wS);

        // State
        switch(wMbi.State)
        {
        case MEM_FREE:
            wS = QString("FREE");
            break;
        case MEM_COMMIT:
            wS = QString("COMM");
            break;
        case MEM_RESERVE:
            wS = QString("RESV");
            break;
        default:
            wS = QString("????");
        }
        setCellContent(wI, 3, wS);

        // Type
        switch(wMbi.Type)
        {
        case MEM_IMAGE:
            wS = QString("IMG");
            break;
        case MEM_MAPPED:
            wS = QString("MAP");
            break;
        case MEM_PRIVATE:
            wS = QString("PRV");
            break;
        default:
            wS = QString("N/A");
            break;
        }
        setCellContent(wI, 3, wS);

        // current access protection
        wS = getProtectionString(wMbi.Protect);
        setCellContent(wI, 4, wS);

        // allocation protection
        wS = getProtectionString(wMbi.AllocationProtect);
        setCellContent(wI, 5, wS);

    }
    if(wMemMapStruct.page != 0)
        BridgeFree(wMemMapStruct.page);
    reloadData(); //refresh memory map
}

void MemoryMapView::stateChangedSlot(DBGSTATE state)
{
    if(state == paused)
        refreshMap();
}

void MemoryMapView::followDumpSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExecDirect(QString("dump " + addr_text).toUtf8().constData());
    emit showCpu();
}

void MemoryMapView::followDisassemblerSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExecDirect(QString("disasm " + addr_text).toUtf8().constData());
    emit showCpu();
}

void MemoryMapView::memoryAccessSingleshootSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpm " + addr_text + ", 0, r").toUtf8().constData());
}

void MemoryMapView::memoryAccessRestoreSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpm " + addr_text + ", 1, r").toUtf8().constData());
}

void MemoryMapView::memoryWriteSingleshootSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpm " + addr_text + ", 0, w").toUtf8().constData());
}

void MemoryMapView::memoryWriteRestoreSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpm " + addr_text + ", 1, w").toUtf8().constData());
}

void MemoryMapView::memoryExecuteSingleshootSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpm " + addr_text + ", 0, x").toUtf8().constData());
}

void MemoryMapView::memoryExecuteRestoreSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpm " + addr_text + ", 1, x").toUtf8().constData());
}

void MemoryMapView::memoryRemoveSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpmc " + addr_text).toUtf8().constData());
}

void MemoryMapView::memoryExecuteSingleshootToggleSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
#ifdef _WIN64
    uint_t selectedAddr = addr_text.toULongLong(0, 16);
#else //x86
    uint_t selectedAddr = addr_text.toULong(0, 16);
#endif //_WIN64
    if((DbgGetBpxTypeAt(selectedAddr)&bp_memory) == bp_memory) //memory breakpoint set
        memoryRemoveSlot();
    else
        memoryExecuteSingleshootSlot();
}

void MemoryMapView::pageMemoryRights()
{
    PageMemoryRights PageMemoryRightsDialog(this);
    connect(&PageMemoryRightsDialog, SIGNAL(refreshMemoryMap()), this, SLOT(refreshMap()));
    uint_t addr = getCellContent(getInitialSelection(), 0).toULongLong(0, 16);
    uint_t size = getCellContent(getInitialSelection(), 1).toULongLong(0, 16);
    PageMemoryRightsDialog.RunAddrSize(addr, size, getCellContent(getInitialSelection(), 3));
}

void MemoryMapView::switchView()
{
    duint setting = 0;
    if(BridgeSettingGetUint("Engine", "ListAllPages", &setting) && setting)
        BridgeSettingSetUint("Engine", "ListAllPages", 0);
    else
        BridgeSettingSetUint("Engine", "ListAllPages", 1);
    DbgSettingsUpdated();
    DbgFunctions()->MemUpdateMap();
    setSingleSelection(0);
    setTableOffset(0);
    stateChangedSlot(paused);
}

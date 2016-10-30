#include <QFileDialog>
#include <QMessageBox>

#include "MemoryMapView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "PageMemoryRights.h"
#include "YaraRuleSelectionDialog.h"
#include "EntropyDialog.h"
#include "HexEditDialog.h"
#include "MiscUtil.h"
#include "GotoDialog.h"
#include "WordEditDialog.h"
#include "VirtualModDialog.h"

MemoryMapView::MemoryMapView(StdTable* parent)
    : StdTable(parent),
      mCipBase(0)
{
    enableMultiSelection(false);

    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Address"), false, tr("Address")); //addr
    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Size"), false, tr("Size")); //size
    addColumnAt(8 + charwidth * 32, tr("Info"), false, tr("Page Information")); //page information
    addColumnAt(8 + charwidth * 5, tr("Type"), false, tr("Allocation Type")); //allocation type
    addColumnAt(8 + charwidth * 11, tr("Protection"), false, tr("Current Protection")); //current protection
    addColumnAt(8 + charwidth * 8, tr("Initial"), false, tr("Allocation Protection")); //allocation protection
    addColumnAt(100, "", false);
    loadColumnFromConfig("MemoryMap");

    connect(Bridge::getBridge(), SIGNAL(updateMemory()), this, SLOT(refreshMap()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(stateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(selectInMemoryMap(duint)), this, SLOT(selectAddress(duint)));
    connect(Bridge::getBridge(), SIGNAL(selectionMemmapGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint, dsint)), this, SLOT(disassembleAtSlot(dsint, dsint)));
    connect(Bridge::getBridge(), SIGNAL(focusMemmap()), this, SLOT(setFocus()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    setupContextMenu();
}

void MemoryMapView::setupContextMenu()
{
    //Follow in Dump
    mFollowDump = new QAction(DIcon("dump.png"), tr("&Follow in Dump"), this);
    connect(mFollowDump, SIGNAL(triggered()), this, SLOT(followDumpSlot()));

    //Follow in Disassembler
    mFollowDisassembly = new QAction(DIcon(ArchValue("processor32.png", "processor64.png")), tr("Follow in &Disassembler"), this);
    connect(mFollowDisassembly, SIGNAL(triggered()), this, SLOT(followDisassemblerSlot()));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(doubleClickedSlot()));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));

    //Yara
    mYara = new QAction(DIcon("yara.png"), "&Yara...", this);
    connect(mYara, SIGNAL(triggered()), this, SLOT(yaraSlot()));

    //Set PageMemory Rights
    mPageMemoryRights = new QAction(DIcon("memmap_set_page_memory_rights.png"), tr("Set Page Memory Rights"), this);
    connect(mPageMemoryRights, SIGNAL(triggered()), this, SLOT(pageMemoryRights()));

    //Switch View
    mSwitchView = new QAction(DIcon("change-view.png"), tr("&Switch View"), this);
    connect(mSwitchView, SIGNAL(triggered()), this, SLOT(switchView()));

    //Breakpoint menu
    mBreakpointMenu = new QMenu(tr("Memory &Breakpoint"), this);
    mBreakpointMenu->setIcon(DIcon("breakpoint.png"));

    //Breakpoint->Memory Access
    mMemoryAccessMenu = new QMenu(tr("Access"), this);
    mMemoryAccessMenu->setIcon(DIcon("breakpoint_memory_access.png"));
    mMemoryAccessSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot.png"), tr("&Singleshoot"), this);
    connect(mMemoryAccessSingleshoot, SIGNAL(triggered()), this, SLOT(memoryAccessSingleshootSlot()));
    mMemoryAccessMenu->addAction(mMemoryAccessSingleshoot);
    mMemoryAccessRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore"), this);
    connect(mMemoryAccessRestore, SIGNAL(triggered()), this, SLOT(memoryAccessRestoreSlot()));
    mMemoryAccessMenu->addAction(mMemoryAccessRestore);
    mBreakpointMenu->addMenu(mMemoryAccessMenu);

    //Breakpoint->Memory Write
    mMemoryWriteMenu = new QMenu(tr("Write"), this);
    mMemoryWriteMenu->setIcon(DIcon("breakpoint_memory_write.png"));
    mMemoryWriteSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot.png"), tr("&Singleshoot"), this);
    connect(mMemoryWriteSingleshoot, SIGNAL(triggered()), this, SLOT(memoryWriteSingleshootSlot()));
    mMemoryWriteMenu->addAction(mMemoryWriteSingleshoot);
    mMemoryWriteRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore"), this);
    connect(mMemoryWriteRestore, SIGNAL(triggered()), this, SLOT(memoryWriteRestoreSlot()));
    mMemoryWriteMenu->addAction(mMemoryWriteRestore);
    mBreakpointMenu->addMenu(mMemoryWriteMenu);

    //Breakpoint->Memory Execute
    mMemoryExecuteMenu = new QMenu(tr("Execute"), this);
    mMemoryExecuteMenu->setIcon(DIcon("breakpoint_memory_execute.png"));
    mMemoryExecuteSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot.png"), tr("&Singleshoot"), this);
    mMemoryExecuteSingleshoot->setShortcutContext(Qt::WidgetShortcut);
    connect(mMemoryExecuteSingleshoot, SIGNAL(triggered()), this, SLOT(memoryExecuteSingleshootSlot()));
    mMemoryExecuteMenu->addAction(mMemoryExecuteSingleshoot);
    mMemoryExecuteRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore"), this);
    connect(mMemoryExecuteRestore, SIGNAL(triggered()), this, SLOT(memoryExecuteRestoreSlot()));
    mMemoryExecuteMenu->addAction(mMemoryExecuteRestore);
    mBreakpointMenu->addMenu(mMemoryExecuteMenu);

    //Breakpoint->Remove
    mMemoryRemove = new QAction(tr("&Remove"), this);
    mMemoryRemove->setShortcutContext(Qt::WidgetShortcut);
    connect(mMemoryRemove, SIGNAL(triggered()), this, SLOT(memoryRemoveSlot()));
    mBreakpointMenu->addAction(mMemoryRemove);

    //Action shortcut action that does something
    mMemoryExecuteSingleshootToggle = new QAction(this);
    mMemoryExecuteSingleshootToggle->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mMemoryExecuteSingleshootToggle);
    connect(mMemoryExecuteSingleshootToggle, SIGNAL(triggered()), this, SLOT(memoryExecuteSingleshootToggleSlot()));

    //Allocate memory
    mMemoryAllocate = new QAction(DIcon("memmap_alloc_memory.png"), tr("&Allocate memory"), this);
    mMemoryAllocate->setShortcutContext(Qt::WidgetShortcut);
    connect(mMemoryAllocate, SIGNAL(triggered()), this, SLOT(memoryAllocateSlot()));
    this->addAction(mMemoryAllocate);

    //Free memory
    mMemoryFree = new QAction(DIcon("memmap_free_memory.png"), tr("&Free memory"), this);
    mMemoryFree->setShortcutContext(Qt::WidgetShortcut);
    connect(mMemoryFree, SIGNAL(triggered()), this, SLOT(memoryFreeSlot()));
    this->addAction(mMemoryFree);

    //Goto
    mGotoMenu = new QMenu(tr("Go to"), this);
    mGotoMenu->setIcon(DIcon("goto.png"));

    //Goto->Origin
    mGotoOrigin = new QAction(DIcon("cbp.png"), tr("Origin"), this);
    mGotoOrigin->setShortcutContext(Qt::WidgetShortcut);
    connect(mGotoOrigin, SIGNAL(triggered()), this, SLOT(gotoOriginSlot()));
    this->addAction(mGotoOrigin);
    mGotoMenu->addAction(mGotoOrigin);

    //Goto->Expression
    mGotoExpression = new QAction(DIcon("geolocation-goto.png"), tr("Expression"), this);
    mGotoExpression->setShortcutContext(Qt::WidgetShortcut);
    connect(mGotoExpression, SIGNAL(triggered()), this, SLOT(gotoExpressionSlot()));
    this->addAction(mGotoExpression);
    mGotoMenu->addAction(mGotoExpression);

    //Entropy
    mEntropy = new QAction(DIcon("entropy.png"), tr("Entropy..."), this);
    mEntropy->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mEntropy);
    connect(mEntropy, SIGNAL(triggered()), this, SLOT(entropy()));

    //Find
    mFindPattern = new QAction(DIcon("search-for.png"), tr("&Find Pattern..."), this);
    this->addAction(mFindPattern);
    mFindPattern->setShortcutContext(Qt::WidgetShortcut);
    connect(mFindPattern, SIGNAL(triggered()), this, SLOT(findPatternSlot()));

    //Dump
    mDumpMemory = new QAction(DIcon("binary_save.png"), tr("&Dump Memory to File"), this);
    connect(mDumpMemory, SIGNAL(triggered()), this, SLOT(dumpMemory()));

    //Add virtual module
    mAddVirtualMod = new QAction(tr("Add virtual module"), this);
    connect(mAddVirtualMod, SIGNAL(triggered()), this, SLOT(addVirtualModSlot()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void MemoryMapView::refreshShortcutsSlot()
{
    mMemoryExecuteSingleshoot->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mMemoryRemove->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mMemoryExecuteSingleshootToggle->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mFindPattern->setShortcut(ConfigShortcut("ActionFindPattern"));
    mGotoOrigin->setShortcut(ConfigShortcut("ActionGotoOrigin"));
    mGotoExpression->setShortcut(ConfigShortcut("ActionGotoExpression"));
    mEntropy->setShortcut(ConfigShortcut("ActionEntropy"));
    mMemoryFree->setShortcut(ConfigShortcut("ActionFreeMemory"));
    mMemoryAllocate->setShortcut(ConfigShortcut("ActionAllocateMemory"));
}

void MemoryMapView::contextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;
    QMenu wMenu(this); //create context menu
    wMenu.addAction(mFollowDisassembly);
    wMenu.addAction(mFollowDump);
    wMenu.addAction(mDumpMemory);
    wMenu.addAction(mYara);
    wMenu.addAction(mEntropy);
    wMenu.addAction(mFindPattern);
    wMenu.addAction(mSwitchView);
    wMenu.addSeparator();
    wMenu.addAction(mMemoryAllocate);
    wMenu.addAction(mMemoryFree);
    wMenu.addAction(mAddVirtualMod);
    wMenu.addMenu(mGotoMenu);
    wMenu.addSeparator();
    wMenu.addAction(mPageMemoryRights);
    wMenu.addSeparator();
    wMenu.addMenu(mBreakpointMenu);
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
    }

    QString wStr = getCellContent(getInitialSelection(), 0);
#ifdef _WIN64
    duint selectedAddr = wStr.toULongLong(0, 16);
#else //x86
    duint selectedAddr = wStr.toULong(0, 16);
#endif //_WIN64
    if((DbgGetBpxTypeAt(selectedAddr) & bp_memory) == bp_memory) //memory breakpoint set
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

    mAddVirtualMod->setVisible(!DbgFunctions()->ModBaseFromAddr(selectedAddr));

    wMenu.exec(mapToGlobal(pos)); //execute context menu
}

QString MemoryMapView::getProtectionString(DWORD Protect)
{
#define RIGHTS_STRING (sizeof("ERWCG") + 1)
    char rights[RIGHTS_STRING];

    if(!DbgFunctions()->PageRightsToString(Protect, rights))
        return "bad";

    return QString(rights);
}

QString MemoryMapView::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if(col == 0) //address
    {
        QString wStr = getCellContent(rowBase + rowOffset, col);
#ifdef _WIN64
        duint addr = wStr.toULongLong(0, 16);
#else //x86
        duint addr = wStr.toULong(0, 16);
#endif //_WIN64
        QColor color = textColor;
        QColor backgroundColor = Qt::transparent;
        bool isBp = (DbgGetBpxTypeAt(addr) & bp_memory) == bp_memory;
        bool isCip = addr == mCipBase;
        if(isCip && isBp)
        {
            color = ConfigColor("MemoryMapBreakpointBackgroundColor");
            backgroundColor = ConfigColor("MemoryMapCipBackgroundColor");
        }
        else if(isBp)
        {
            color = ConfigColor("MemoryMapBreakpointColor");
            backgroundColor = ConfigColor("MemoryMapBreakpointBackgroundColor");
        }
        else if(isCip)
        {
            color = ConfigColor("MemoryMapCipColor");
            backgroundColor = ConfigColor("MemoryMapCipBackgroundColor");
        }
        else if(isSelected(rowBase, rowOffset) == true)
            painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor));

        if(backgroundColor.alpha())
            painter->fillRect(QRect(x, y, w - 1, h), QBrush(backgroundColor));
        painter->setPen(color);
        painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
        return "";
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
        QString wStr = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);;
        if(!ConfigBool("Engine", "ListAllPages"))
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
        wS = ToPtrString((duint)wMbi.BaseAddress);
        setCellContent(wI, 0, wS);

        // Size
        wS = ToPtrString((duint)wMbi.RegionSize);
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
    DbgCmdExec(QString("dump %1").arg(getCellContent(getInitialSelection(), 0)).toUtf8().constData());
}

void MemoryMapView::followDisassemblerSlot()
{
    DbgCmdExec(QString("disasm %1").arg(getCellContent(getInitialSelection(), 0)).toUtf8().constData());
}

void MemoryMapView::doubleClickedSlot()
{
    auto addr = DbgValFromString(getCellContent(getInitialSelection(), 0).toUtf8().constData());
    if(!addr)
        return;
    if(DbgFunctions()->MemIsCodePage(addr, false))
        followDisassemblerSlot();
    else
        followDumpSlot();
}

void MemoryMapView::yaraSlot()
{
    YaraRuleSelectionDialog yaraDialog(this);
    if(yaraDialog.exec() == QDialog::Accepted)
    {
        QString addr_text = getCellContent(getInitialSelection(), 0);
        QString size_text = getCellContent(getInitialSelection(), 1);
        DbgCmdExec(QString("yara \"%0\",%1,%2").arg(yaraDialog.getSelectedFile()).arg(addr_text).arg(size_text).toUtf8().constData());
        emit showReferences();
    }
}

void MemoryMapView::memoryAccessSingleshootSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpm " + addr_text + ", 0, a").toUtf8().constData());
}

void MemoryMapView::memoryAccessRestoreSlot()
{
    QString addr_text = getCellContent(getInitialSelection(), 0);
    DbgCmdExec(QString("bpm " + addr_text + ", 1, a").toUtf8().constData());
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
    duint selectedAddr = addr_text.toULongLong(0, 16);
#else //x86
    duint selectedAddr = addr_text.toULong(0, 16);
#endif //_WIN64
    if((DbgGetBpxTypeAt(selectedAddr) & bp_memory) == bp_memory) //memory breakpoint set
        memoryRemoveSlot();
    else
        memoryExecuteSingleshootSlot();
}

void MemoryMapView::pageMemoryRights()
{
    PageMemoryRights PageMemoryRightsDialog(this);
    connect(&PageMemoryRightsDialog, SIGNAL(refreshMemoryMap()), this, SLOT(refreshMap()));
    duint addr = getCellContent(getInitialSelection(), 0).toULongLong(0, 16);
    duint size = getCellContent(getInitialSelection(), 1).toULongLong(0, 16);
    PageMemoryRightsDialog.RunAddrSize(addr, size, getCellContent(getInitialSelection(), 3));
}

void MemoryMapView::switchView()
{
    Config()->setBool("Engine", "ListAllPages", !ConfigBool("Engine", "ListAllPages"));
    Config()->writeBools();
    DbgSettingsUpdated();
    DbgFunctions()->MemUpdateMap();
    setSingleSelection(0);
    setTableOffset(0);
    stateChangedSlot(paused);
}

void MemoryMapView::entropy()
{
    duint addr = getCellContent(getInitialSelection(), 0).toULongLong(0, 16);
    duint size = getCellContent(getInitialSelection(), 1).toULongLong(0, 16);
    unsigned char* data = new unsigned char[size];
    DbgMemRead(addr, data, size);

    EntropyDialog entropyDialog(this);
    entropyDialog.setWindowTitle(tr("Entropy (Address: %1, Size: %2)").arg(ToPtrString(addr).arg(ToPtrString(size))));
    entropyDialog.show();
    entropyDialog.GraphMemory(data, size);
    entropyDialog.exec();

    delete[] data;
}

void MemoryMapView::memoryAllocateSlot()
{
    WordEditDialog mLineEdit(this);
    mLineEdit.setup(tr("Size"), 0x1000, sizeof(duint));
    if(mLineEdit.exec() == QDialog::Accepted)
    {
        duint memsize = mLineEdit.getVal();
        if(memsize == 0) // 1GB
        {
            SimpleWarningBox(this, tr("Warning"), tr("You're trying to allocate a zero-sized buffer just now."));
            return;
        }
        if(memsize > 1024 * 1024 * 1024)
        {
            SimpleErrorBox(this, tr("Error"), tr("The size of buffer you're trying to allocate exceeds 1GB. Please check your expression to ensure nothing is wrong."));
            return;
        }
        DbgCmdExecDirect(QString("alloc %1").arg(ToPtrString(memsize)).toUtf8().constData());
        duint addr = DbgValFromString("$result");
        if(addr != 0)
        {
            DbgCmdExec("dump $result");
        }
        else
        {
            SimpleErrorBox(this, tr("Error"), tr("Memory allocation failed!"));
            return;
        }
    }
}

void MemoryMapView::memoryFreeSlot()
{
    DbgCmdExec(QString("free %1").arg(getCellContent(getInitialSelection(), 0)).toUtf8().constData());
}

void MemoryMapView::findPatternSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    duint addr = getCellContent(getInitialSelection(), 0).toULongLong(0, 16);
    if(hexEdit.entireBlock())
        addr = 0;
    QString addrText = ToPtrString(addr);
    DbgCmdExec(QString("findmemall " + addrText + ", \"" + hexEdit.mHexEdit->pattern() + "\", &data&").toUtf8().constData());
    emit showReferences();
}

void MemoryMapView::dumpMemory()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Memory Region"), QDir::currentPath(), tr("All files (*.*)"));

    if(fileName.length())
    {
        fileName = QDir::toNativeSeparators(fileName);
        QString cmd = QString("savedata ""%1"",%2,%3").arg(fileName, getCellContent(getInitialSelection(), 0),
                      getCellContent(getInitialSelection(), 1));
        DbgCmdExec(cmd.toUtf8().constData());
    }
}

void MemoryMapView::selectAddress(duint va)
{
    auto base = DbgMemFindBaseAddr(va, nullptr);
    if(base)
    {
        auto baseText = ToPtrString(base);
        auto rows = getRowCount();
        for(dsint row = 0; row < rows; row++)
            if(getCellContent(row, 0) == baseText)
            {
                scrollSelect(row);
                reloadData();
                return;
            }
    }
    QMessageBox msg(QMessageBox::Critical, tr("Error"), tr("Address %0 not found in memory map...").arg(ToPtrString(va)));
    msg.setWindowIcon(DIcon("compile-error.png"));
    msg.exec();
    QMessageBox::warning(this, tr("Error"), QString());
}

void MemoryMapView::gotoOriginSlot()
{
    selectAddress(mCipBase);
}

void MemoryMapView::gotoExpressionSlot()
{
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->setWindowTitle(tr("Enter the address to find..."));
    if(mGoto->exec() == QDialog::Accepted)
    {
        selectAddress(DbgValFromString(mGoto->expressionText.toUtf8().constData()));
    }
}

void MemoryMapView::addVirtualModSlot()
{
    auto base = duint(getCellContent(getInitialSelection(), 0).toULongLong(nullptr, 16));
    auto size = duint(getCellContent(getInitialSelection(), 1).toULongLong(nullptr, 16));
    VirtualModDialog mDialog(this);
    mDialog.setData("", base, size);
    if(mDialog.exec() != QDialog::Accepted)
        return;
    QString modname;
    if(!mDialog.getData(modname, base, size))
        return;
    DbgCmdExec(QString("virtualmod \"%1\", %2, %3").arg(modname).arg(ToHexString(base)).arg(ToHexString(size)).toUtf8().constData());
}

void MemoryMapView::selectionGetSlot(SELECTIONDATA* selection)
{
    selection->start = selection->end = duint(getCellContent(getInitialSelection(), 0).toULongLong(nullptr, 16));
    Bridge::getBridge()->setResult(1);
}

void MemoryMapView::disassembleAtSlot(dsint va, dsint cip)
{
    Q_UNUSED(va)
    mCipBase = DbgMemFindBaseAddr(cip, nullptr);;
}

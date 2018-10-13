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
#include "LineEditDialog.h"

MemoryMapView::MemoryMapView(StdTable* parent)
    : StdTable(parent),
      mCipBase(0)
{
    setDrawDebugOnly(true);
    enableMultiSelection(true);
    noDisassemblyPopup();

    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Address"), true, tr("Address")); //addr
    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Size"), false, tr("Size")); //size
    addColumnAt(8 + charwidth * 32, tr("Info"), false, tr("Page Information")); //page information
    addColumnAt(8 + charwidth * 28, tr("Content"), false, tr("Content of section")); //content of section
    addColumnAt(8 + charwidth * 5, tr("Type"), true, tr("Allocation Type")); //allocation type
    addColumnAt(8 + charwidth * 11, tr("Protection"), true, tr("Current Protection")); //current protection
    addColumnAt(8 + charwidth * 8, tr("Initial"), true, tr("Allocation Protection")); //allocation protection
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
    mYara->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mYara);
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
    makeCommandAction(mMemoryAccessSingleshoot, "bpm $, 0, a");
    mMemoryAccessMenu->addAction(mMemoryAccessSingleshoot);
    mMemoryAccessRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore"), this);
    makeCommandAction(mMemoryAccessRestore, "bpm $, 1, a");
    mMemoryAccessMenu->addAction(mMemoryAccessRestore);
    mBreakpointMenu->addMenu(mMemoryAccessMenu);

    //Breakpoint->Memory Read
    mMemoryReadMenu = new QMenu(tr("Read"), this);
    mMemoryReadMenu->setIcon(DIcon("breakpoint_memory_read.png"));
    mMemoryReadSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot.png"), tr("&Singleshoot"), this);
    makeCommandAction(mMemoryReadSingleshoot, "bpm $, 0, r");
    mMemoryReadMenu->addAction(mMemoryReadSingleshoot);
    mMemoryReadRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore"), this);
    makeCommandAction(mMemoryReadRestore, "bpm $, 1, r");
    mMemoryReadMenu->addAction(mMemoryReadRestore);
    mBreakpointMenu->addMenu(mMemoryReadMenu);

    //Breakpoint->Memory Write
    mMemoryWriteMenu = new QMenu(tr("Write"), this);
    mMemoryWriteMenu->setIcon(DIcon("breakpoint_memory_write.png"));
    mMemoryWriteSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot.png"), tr("&Singleshoot"), this);
    makeCommandAction(mMemoryWriteSingleshoot, "bpm $, 0, w");
    mMemoryWriteMenu->addAction(mMemoryWriteSingleshoot);
    mMemoryWriteRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore"), this);
    makeCommandAction(mMemoryWriteRestore, "bpm $, 1, w");
    mMemoryWriteMenu->addAction(mMemoryWriteRestore);
    mBreakpointMenu->addMenu(mMemoryWriteMenu);

    //Breakpoint->Memory Execute
    mMemoryExecuteMenu = new QMenu(tr("Execute"), this);
    mMemoryExecuteMenu->setIcon(DIcon("breakpoint_memory_execute.png"));
    mMemoryExecuteSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot.png"), tr("&Singleshoot"), this);
    makeCommandAction(mMemoryExecuteSingleshoot, "bpm $, 0, x");
    mMemoryExecuteMenu->addAction(mMemoryExecuteSingleshoot);
    mMemoryExecuteRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit.png"), tr("&Restore"), this);
    makeCommandAction(mMemoryExecuteRestore, "bpm $, 1, x");
    mMemoryExecuteMenu->addAction(mMemoryExecuteRestore);
    mBreakpointMenu->addMenu(mMemoryExecuteMenu);

    //Breakpoint->Remove
    mMemoryRemove = new QAction(tr("&Remove"), this);
    mMemoryRemove->setShortcutContext(Qt::WidgetShortcut);
    makeCommandAction(mMemoryRemove, "bpmc $");
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
    makeCommandAction(mMemoryFree, "free $");
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
    mAddVirtualMod = new QAction(DIcon("virtual.png"), tr("Add virtual module"), this);
    connect(mAddVirtualMod, SIGNAL(triggered()), this, SLOT(addVirtualModSlot()));

    //Comment
    mComment = new QAction(DIcon("comment.png"), tr("&Comment"), this);
    this->addAction(mComment);
    connect(mComment, SIGNAL(triggered()), this, SLOT(commentSlot()));
    mComment->setShortcutContext(Qt::WidgetShortcut);

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
    mYara->setShortcut(ConfigShortcut("ActionYara"));
    mComment->setShortcut(ConfigShortcut("ActionSetComment"));
}

void MemoryMapView::contextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;
    QMenu wMenu(this); //create context menu
    wMenu.addAction(mFollowDisassembly);
    wMenu.addAction(mFollowDump);
    wMenu.addAction(mDumpMemory);
    wMenu.addAction(mComment);
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
        mMemoryReadMenu->menuAction()->setVisible(false);
        mMemoryWriteMenu->menuAction()->setVisible(false);
        mMemoryExecuteMenu->menuAction()->setVisible(false);
        mMemoryRemove->setVisible(true);
    }
    else //memory breakpoint not set
    {
        mMemoryAccessMenu->menuAction()->setVisible(true);
        mMemoryReadMenu->menuAction()->setVisible(true);
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
        QColor color = mTextColor;
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
            painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));

        if(backgroundColor.alpha())
            painter->fillRect(QRect(x, y, w - 1, h), QBrush(backgroundColor));
        painter->setPen(color);
        painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
        return QString();
    }
    else if(col == 2) //info
    {
        QString wStr = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);;
        if(wStr.startsWith(" \""))
        {
            painter->setPen(ConfigColor("MemoryMapSectionTextColor"));
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
            return QString();
        }
    }
    else if(col == 4) //CPROT
    {
        QString wStr = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);;
        if(!ConfigBool("Engine", "ListAllPages"))
        {
            painter->setPen(ConfigColor("MemoryMapSectionTextColor"));
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
            return QString();
        }
    }
    return StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
}

QAction* MemoryMapView::makeCommandAction(QAction* action, const QString & command)
{
    action->setData(QVariant(command));
    connect(action, SIGNAL(triggered()), this, SLOT(ExecCommand()));
    return action;
}

/**
 * @brief MemoryMapView::ExecCommand execute command slot for menus.
 */
void MemoryMapView::ExecCommand()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action)
    {
        QString command = action->data().toString();
        if(command.contains('$'))
        {
            for(int i : getSelection())
            {
                QString specializedCommand = command;
                specializedCommand.replace(QChar('$'), getCellContent(i, 0)); // $ -> Base address
                DbgCmdExec(specializedCommand.toUtf8().constData());
            }
        }
        else
            DbgCmdExec(command.toUtf8().constData());
    }
}

void MemoryMapView::refreshMap()
{
    MEMMAP wMemMapStruct;
    int wI;

    memset(&wMemMapStruct, 0, sizeof(MEMMAP));

    DbgMemMap(&wMemMapStruct);

    setRowCount(wMemMapStruct.count);

    QString wS;
    MEMORY_BASIC_INFORMATION wMbi;
    for(wI = 0; wI < wMemMapStruct.count; wI++)
    {
        wMbi = (wMemMapStruct.page)[wI].mbi;

        // Base address
        setCellContent(wI, 0, ToPtrString((duint)wMbi.BaseAddress));

        // Size
        setCellContent(wI, 1, ToPtrString((duint)wMbi.RegionSize));

        // Information
        wS = QString((wMemMapStruct.page)[wI].info);
        setCellContent(wI, 2, wS);

        // Content, TODO: proper section content analysis in dbg/memory.cpp:MemUpdateMap
        char comment_text[MAX_COMMENT_SIZE];
        if(DbgFunctions()->GetUserComment((duint)wMbi.BaseAddress, comment_text)) // user comment present
            wS = comment_text;
        else if(wS.contains(".bss"))
            wS = tr("Uninitialized data");
        else if(wS.contains(".data"))
            wS = tr("Initialized data");
        else if(wS.contains(".edata"))
            wS = tr("Export tables");
        else if(wS.contains(".idata"))
            wS = tr("Import tables");
        else if(wS.contains(".pdata"))
            wS = tr("Exception information");
        else if(wS.contains(".rdata"))
            wS = tr("Read-only initialized data");
        else if(wS.contains(".reloc"))
            wS = tr("Base relocations");
        else if(wS.contains(".rsrc"))
            wS = tr("Resources");
        else if(wS.contains(".text"))
            wS = tr("Executable code");
        else if(wS.contains(".tls"))
            wS = tr("Thread-local storage");
        else if(wS.contains(".xdata"))
            wS = tr("Exception information");
        else
            wS = QString("");
        setCellContent(wI, 3, wS);

        // Type
        const char* type = "";
        switch(wMbi.Type)
        {
        case MEM_IMAGE:
            type = "IMG";
            break;
        case MEM_MAPPED:
            type = "MAP";
            break;
        case MEM_PRIVATE:
            type = "PRV";
            break;
        default:
            type = "N/A";
            break;
        }
        setCellContent(wI, 4, type);

        // current access protection
        setCellContent(wI, 5, getProtectionString(wMbi.Protect));

        // allocation protection
        setCellContent(wI, 6, getProtectionString(wMbi.AllocationProtect));

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
    DbgCmdExecDirect(QString("dump %1").arg(getCellContent(getInitialSelection(), 0)).toUtf8().constData());
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
    {
        followDumpSlot();
        emit Bridge::getBridge()->getDumpAttention();
    }
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

void MemoryMapView::memoryExecuteSingleshootToggleSlot()
{
    for(int i : getSelection())
    {
        QString addr_text = getCellContent(i, 0);
#ifdef _WIN64
        duint selectedAddr = addr_text.toULongLong(0, 16);
#else //x86
        duint selectedAddr = addr_text.toULong(0, 16);
#endif //_WIN64
        if((DbgGetBpxTypeAt(selectedAddr) & bp_memory) == bp_memory) //memory breakpoint set
            DbgCmdExec(QString("bpmc ") + addr_text);
        else
            DbgCmdExec(QString("bpm %1, 0, x").arg(addr_text));
    }
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
            DbgCmdExec("dump $result");
        else
            SimpleErrorBox(this, tr("Error"), tr("Memory allocation failed!"));
    }
}

void MemoryMapView::findPatternSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.isDataCopiable(false);
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
    char modname[MAX_MODULE_SIZE] = "";
    if(!DbgFunctions()->ModNameFromAddr(DbgEval("mod.main()"), modname, false))
        *modname = '\0';
    auto addr = getCellContent(getInitialSelection(), 0);
    QString defaultFile = QString("%1/%2%3.bin").arg(QDir::currentPath(), *modname ? modname +  QString("_") : "", addr);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Memory Region"), defaultFile, tr("Binary files (*.bin);;All files (*.*)"));

    if(fileName.length())
    {
        fileName = QDir::toNativeSeparators(fileName);
        QString cmd = QString("savedata \"%1\",%2,%3").arg(fileName, addr, getCellContent(getInitialSelection(), 1));
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
    SimpleErrorBox(this, tr("Error"), tr("Address %0 not found in memory map...").arg(ToPtrString(va)));
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
    mGoto->setInitialExpression(ToPtrString(duint(getCellContent(getInitialSelection(), 0).toULongLong(nullptr, 16))));
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

void MemoryMapView::commentSlot()
{
    duint wVA = duint(getCellContent(getInitialSelection(), 0).toULongLong(nullptr, 16));
    LineEditDialog mLineEdit(this);
    QString addr_text = ToPtrString(wVA);
    char comment_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetCommentAt((duint)wVA, comment_text))
    {
        if(comment_text[0] == '\1') //automatic comment
            mLineEdit.setText(QString(comment_text + 1));
        else
            mLineEdit.setText(QString(comment_text));
    }
    mLineEdit.setWindowTitle(tr("Add comment at ") + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    if(!DbgSetCommentAt(wVA, mLineEdit.editText.replace('\r', "").replace('\n', "").toUtf8().constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetCommentAt failed!"));

    GuiUpdateMemoryView();
}

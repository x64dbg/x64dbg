#include <QFileDialog>
#include <QMessageBox>

#include "MemoryMapView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "PageMemoryRights.h"
#include "HexEditDialog.h"
#include "MiscUtil.h"
#include "GotoDialog.h"
#include "WordEditDialog.h"
#include "VirtualModDialog.h"
#include "LineEditDialog.h"
#include "RichTextPainter.h"

MemoryMapView::MemoryMapView(StdTable* parent)
    : StdIconTable(parent),
      mCipBase(0)
{
    setDrawDebugOnly(true);
    enableMultiSelection(true);
    setDisassemblyPopupEnabled(false);

    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Address"), true, tr("Address")); //addr
    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Size"), false, tr("Size")); //size
    addColumnAt(charwidth * 9, tr("Party"), false); // party
    addColumnAt(8 + charwidth * 32, tr("Info"), false, tr("Page Information")); //page information
    addColumnAt(8 + charwidth * 28, tr("Content"), false, tr("Content of section")); //content of section
    addColumnAt(8 + charwidth * 5, tr("Type"), true, tr("Allocation Type")); //allocation type
    addColumnAt(8 + charwidth * 11, tr("Protection"), true, tr("Current Protection")); //current protection
    addColumnAt(8 + charwidth * 8, tr("Initial"), true, tr("Allocation Protection")); //allocation protection
    loadColumnFromConfig("MemoryMap");
    setIconColumn(ColParty);

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
    mFollowDump = new QAction(DIcon("dump"), tr("&Follow in Dump"), this);
    connect(mFollowDump, SIGNAL(triggered()), this, SLOT(followDumpSlot()));

    //Follow in Disassembler
    mFollowDisassembly = new QAction(DIcon(ArchValue("processor32", "processor64")), tr("Follow in &Disassembler"), this);
    connect(mFollowDisassembly, SIGNAL(triggered()), this, SLOT(followDisassemblerSlot()));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(doubleClickedSlot()));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));

    //Follow in Symbols
    mFollowSymbols = new QAction(DIcon("pdb"), tr("&Follow in Symbols"), this);
    connect(mFollowSymbols, SIGNAL(triggered()), this, SLOT(followSymbolsSlot()));

    //Set PageMemory Rights
    mPageMemoryRights = new QAction(DIcon("memmap_set_page_memory_rights"), tr("Set Page Memory Rights"), this);
    connect(mPageMemoryRights, SIGNAL(triggered()), this, SLOT(pageMemoryRights()));

    //Switch View
    mSwitchView = new QAction(DIcon("change-view"), "", this);
    setSwitchViewName();
    connect(mSwitchView, SIGNAL(triggered()), this, SLOT(switchView()));

    //Breakpoint menu
    mBreakpointMenu = new QMenu(tr("Memory &Breakpoint"), this);
    mBreakpointMenu->setIcon(DIcon("breakpoint"));

    //Breakpoint->Memory Access
    mMemoryAccessMenu = new QMenu(tr("Access"), this);
    mMemoryAccessMenu->setIcon(DIcon("breakpoint_memory_access"));
    mMemoryAccessSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), this);
    makeCommandAction(mMemoryAccessSingleshoot, "bpm $, 0, a");
    mMemoryAccessMenu->addAction(mMemoryAccessSingleshoot);
    mMemoryAccessRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore"), this);
    makeCommandAction(mMemoryAccessRestore, "bpm $, 1, a");
    mMemoryAccessMenu->addAction(mMemoryAccessRestore);
    mBreakpointMenu->addMenu(mMemoryAccessMenu);

    //Breakpoint->Memory Read
    mMemoryReadMenu = new QMenu(tr("Read"), this);
    mMemoryReadMenu->setIcon(DIcon("breakpoint_memory_read"));
    mMemoryReadSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), this);
    makeCommandAction(mMemoryReadSingleshoot, "bpm $, 0, r");
    mMemoryReadMenu->addAction(mMemoryReadSingleshoot);
    mMemoryReadRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore"), this);
    makeCommandAction(mMemoryReadRestore, "bpm $, 1, r");
    mMemoryReadMenu->addAction(mMemoryReadRestore);
    mBreakpointMenu->addMenu(mMemoryReadMenu);

    //Breakpoint->Memory Write
    mMemoryWriteMenu = new QMenu(tr("Write"), this);
    mMemoryWriteMenu->setIcon(DIcon("breakpoint_memory_write"));
    mMemoryWriteSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), this);
    makeCommandAction(mMemoryWriteSingleshoot, "bpm $, 0, w");
    mMemoryWriteMenu->addAction(mMemoryWriteSingleshoot);
    mMemoryWriteRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore"), this);
    makeCommandAction(mMemoryWriteRestore, "bpm $, 1, w");
    mMemoryWriteMenu->addAction(mMemoryWriteRestore);
    mBreakpointMenu->addMenu(mMemoryWriteMenu);

    //Breakpoint->Memory Execute
    mMemoryExecuteMenu = new QMenu(tr("Execute"), this);
    mMemoryExecuteMenu->setIcon(DIcon("breakpoint_memory_execute"));
    mMemoryExecuteSingleshoot = new QAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), this);
    makeCommandAction(mMemoryExecuteSingleshoot, "bpm $, 0, x");
    mMemoryExecuteMenu->addAction(mMemoryExecuteSingleshoot);
    mMemoryExecuteRestore = new QAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore"), this);
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
    mMemoryAllocate = new QAction(DIcon("memmap_alloc_memory"), tr("&Allocate memory"), this);
    mMemoryAllocate->setShortcutContext(Qt::WidgetShortcut);
    connect(mMemoryAllocate, SIGNAL(triggered()), this, SLOT(memoryAllocateSlot()));
    this->addAction(mMemoryAllocate);

    //Free memory
    mMemoryFree = new QAction(DIcon("memmap_free_memory"), tr("&Free memory"), this);
    mMemoryFree->setShortcutContext(Qt::WidgetShortcut);
    makeCommandAction(mMemoryFree, "free $");
    this->addAction(mMemoryFree);

    //Goto
    mGotoMenu = new QMenu(tr("Go to"), this);
    mGotoMenu->setIcon(DIcon("goto"));

    //Goto->Origin
    mGotoOrigin = new QAction(DIcon("cbp"), ArchValue("EIP", "RIP"), this);
    mGotoOrigin->setShortcutContext(Qt::WidgetShortcut);
    connect(mGotoOrigin, SIGNAL(triggered()), this, SLOT(gotoOriginSlot()));
    this->addAction(mGotoOrigin);
    mGotoMenu->addAction(mGotoOrigin);

    //Goto->Expression
    mGotoExpression = new QAction(DIcon("geolocation-goto"), tr("Expression"), this);
    mGotoExpression->setShortcutContext(Qt::WidgetShortcut);
    connect(mGotoExpression, SIGNAL(triggered()), this, SLOT(gotoExpressionSlot()));
    this->addAction(mGotoExpression);
    mGotoMenu->addAction(mGotoExpression);

    //Find
    mFindPattern = new QAction(DIcon("search-for"), tr("&Find Pattern..."), this);
    this->addAction(mFindPattern);
    mFindPattern->setShortcutContext(Qt::WidgetShortcut);
    connect(mFindPattern, SIGNAL(triggered()), this, SLOT(findPatternSlot()));

    //Dump
    //TODO: These two actions should also appear in CPUDump
    mDumpMemory = new QAction(DIcon("binary_save"), tr("&Dump Memory to File"), this);
    connect(mDumpMemory, SIGNAL(triggered()), this, SLOT(dumpMemory()));

    //Load
    mLoadMemory = new QAction(DIcon(""), tr("&Overwrite with Data from File"), this);
    connect(mLoadMemory, SIGNAL(triggered()), this, SLOT(loadMemory()));

    //Add virtual module
    mAddVirtualMod = new QAction(DIcon("virtual"), tr("Add virtual module"), this);
    connect(mAddVirtualMod, SIGNAL(triggered()), this, SLOT(addVirtualModSlot()));

    //References
    mReferences = new QAction(DIcon("find"), tr("Find references to region"), this);
    connect(mReferences, SIGNAL(triggered()), this, SLOT(findReferencesSlot()));

    //Comment
    mComment = new QAction(DIcon("comment"), tr("&Comment"), this);
    this->addAction(mComment);
    connect(mComment, SIGNAL(triggered()), this, SLOT(commentSlot()));
    mComment->setShortcutContext(Qt::WidgetShortcut);

    mPluginMenu = new QMenu(this);
    Bridge::getBridge()->emitMenuAddToList(this, mPluginMenu, GUI_MEMMAP_MENU);

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
    mMemoryFree->setShortcut(ConfigShortcut("ActionFreeMemory"));
    mMemoryAllocate->setShortcut(ConfigShortcut("ActionAllocateMemory"));
    mComment->setShortcut(ConfigShortcut("ActionSetComment"));
}

void MemoryMapView::contextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;

    duint selectedAddr = getSelectionAddr();

    QMenu wMenu(this); //create context menu
    wMenu.addAction(mFollowDisassembly);
    wMenu.addAction(mFollowDump);

    if(DbgFunctions()->ModBaseFromAddr(selectedAddr))
        wMenu.addAction(mFollowSymbols);

    wMenu.addAction(mDumpMemory);
    //wMenu.addAction(mLoadMemory); //TODO:loaddata command
    wMenu.addAction(mComment);
    wMenu.addAction(mFindPattern);
    wMenu.addAction(mSwitchView);
    wMenu.addAction(mReferences);
    wMenu.addSeparator();
    wMenu.addAction(mMemoryAllocate);
    wMenu.addAction(mMemoryFree);
    wMenu.addAction(mAddVirtualMod);
    wMenu.addMenu(mGotoMenu);
    wMenu.addSeparator();
    wMenu.addAction(mPageMemoryRights);
    wMenu.addSeparator();
    wMenu.addMenu(mBreakpointMenu);
    wMenu.addSeparator();
    DbgMenuPrepare(GUI_MEMMAP_MENU);
    wMenu.addActions(mPluginMenu->actions());
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy"));
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
    }

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

static QString getProtectionString(DWORD Protect)
{
#define RIGHTS_STRING (sizeof("ERWCG"))
    char rights[RIGHTS_STRING];

    if(!DbgFunctions()->PageRightsToString(Protect, rights))
        return "bad";

    return QString(rights);
}

QString MemoryMapView::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if(col == 0) //address
    {
        int row = rowBase + rowOffset;
        auto addr = getCellUserdata(row, ColAddress);
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
        QString wStr = getCellContent(rowBase + rowOffset, col);
        painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
        return QString();
    }
    else if(col == ColPageInfo) //info
    {
        QString wStr = StdIconTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
        auto addr = getCellUserdata(rowBase + rowOffset, ColAddress);
        if(wStr.contains(" \""))
        {
            auto idx = wStr.indexOf(" \"");
            auto pre = wStr.mid(0, idx);
            auto post = wStr.mid(idx);
            RichTextPainter::List richText;
            RichTextPainter::CustomRichText_t entry;
            entry.flags = RichTextPainter::FlagColor;
            if(!pre.isEmpty())
            {
                entry.text = pre;
                entry.textColor = mTextColor;
                richText.push_back(entry);
            }
            entry.text = post;
            entry.textColor = ConfigColor("MemoryMapSectionTextColor");
            richText.push_back(entry);
            RichTextPainter::paintRichText(painter, x, y, getColumnWidth(col), getRowHeight(), 4, richText, mFontMetrics);
            return QString();
        }
        else if(DbgFunctions()->ModBaseFromAddr(addr) == addr) // module header page
        {
            auto party = DbgFunctions()->ModGetParty(addr);
            painter->setPen(ConfigColor(party == mod_user ? "SymbolUserTextColor" : "SymbolSystemTextColor"));
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
            return QString();
        }
    }
    else if(col == ColCurProtect) //CPROT
    {
        QString wStr = StdIconTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);;
        if(!ConfigBool("Engine", "ListAllPages"))
        {
            painter->setPen(ConfigColor("MemoryMapSectionTextColor"));
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
            return QString();
        }
    }
    return StdIconTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
}

void MemoryMapView::setSwitchViewName()
{
    if(ConfigBool("Engine", "ListAllPages"))
    {
        mSwitchView->setText(tr("Section &view"));
    }
    else
    {
        mSwitchView->setText(tr("Region &view"));
    }
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
                specializedCommand.replace(QChar('$'), ToHexString(getCellUserdata(i, ColAddress))); // $ -> Base address
                DbgCmdExec(specializedCommand);
            }
        }
        else
            DbgCmdExec(command);
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
        setCellContent(wI, ColAddress, ToPtrString((duint)wMbi.BaseAddress));
        setCellUserdata(wI, ColAddress, (duint)wMbi.BaseAddress);

        // Size
        setCellContent(wI, ColSize, ToPtrString((duint)wMbi.RegionSize));
        setCellUserdata(wI, ColSize, (duint)wMbi.RegionSize);

        // Party
        int party = DbgFunctions()->ModGetParty((duint)wMbi.BaseAddress);
        switch(party)
        {
        case mod_user:
            setCellContent(wI, ColParty, tr("User"));
            setRowIcon(wI, DIcon("markasuser"));
            break;
        case mod_system:
            setCellContent(wI, ColParty, tr("System"));
            setRowIcon(wI, DIcon("markassystem"));
            break;
        default:
            setCellContent(wI, ColParty, QString::number(party));
            setRowIcon(wI, DIcon("markasparty"));
            break;
        }

        // Information
        wS = QString((wMemMapStruct.page)[wI].info);
        setCellContent(wI, ColPageInfo, wS);

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
        setCellContent(wI, ColContent, std::move(wS));

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
        setCellContent(wI, ColAllocation, type);

        // current access protection
        setCellContent(wI, ColCurProtect, getProtectionString(wMbi.Protect));

        // allocation protection
        setCellContent(wI, ColAllocProtect, getProtectionString(wMbi.AllocationProtect));

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
    DbgCmdExecDirect(QString("dump %1").arg(getSelectionText()));
}

void MemoryMapView::followDisassemblerSlot()
{
    DbgCmdExec(QString("disasm %1").arg(getSelectionText()));
}

void MemoryMapView::followSymbolsSlot()
{
    DbgCmdExec(QString("symfollow %1").arg(getSelectionText()));
}

void MemoryMapView::doubleClickedSlot()
{
    auto addr = DbgValFromString(getSelectionText().toUtf8().constData());
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

void MemoryMapView::memoryExecuteSingleshootToggleSlot()
{
    for(int i : getSelection())
    {
        QString addr_text = getCellContent(i, ColAddress);
        duint selectedAddr = getSelectionAddr();
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
    duint addr = getSelectionAddr();
    duint size = getCellUserdata(getInitialSelection(), ColSize);
    PageMemoryRightsDialog.RunAddrSize(addr, size, getCellContent(getInitialSelection(), ColCurProtect));
}

void MemoryMapView::switchView()
{
    Config()->setBool("Engine", "ListAllPages", !ConfigBool("Engine", "ListAllPages"));
    Config()->writeBools();
    DbgSettingsUpdated();
    setSwitchViewName();
    DbgFunctions()->MemUpdateMap();
    setSingleSelection(0);
    setTableOffset(0);
    stateChangedSlot(paused);
}

void MemoryMapView::memoryAllocateSlot()
{
    WordEditDialog mLineEdit(this);
    mLineEdit.setup(tr("Size"), 0x1000, sizeof(duint));
    if(mLineEdit.exec() == QDialog::Accepted)
    {
        duint memsize = mLineEdit.getVal();
        if(memsize == 0)
        {
            SimpleWarningBox(this, tr("Warning"), tr("You're trying to allocate a zero-sized buffer just now."));
            return;
        }
        if(memsize > 1024 * 1024 * 1024) // 1GB
        {
            SimpleErrorBox(this, tr("Error"), tr("The size of buffer you're trying to allocate exceeds 1GB. Please check your expression to ensure nothing is wrong."));
            return;
        }
        DbgCmdExecDirect(QString("alloc %1").arg(ToPtrString(memsize)));
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
    duint entireBlockEnabled = 0;
    BridgeSettingGetUint("Gui", "MemoryMapEntireBlock", &entireBlockEnabled);
    hexEdit.showEntireBlock(true, entireBlockEnabled);
    hexEdit.showKeepSize(false);
    hexEdit.isDataCopiable(false);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    duint addr = getSelectionAddr();
    entireBlockEnabled = hexEdit.entireBlock();
    BridgeSettingSetUint("Gui", "MemoryMapEntireBlock", entireBlockEnabled);
    if(entireBlockEnabled)
        addr = 0;
    DbgCmdExec(QString("findallmem %1, %2, &data&").arg(ToPtrString(addr)).arg(hexEdit.mHexEdit->pattern()));
    emit showReferences();
}

void MemoryMapView::dumpMemory()
{
    duint start = 0;
    duint end = 0;
    for(auto row : getSelection())
    {
        auto base = getCellUserdata(row, ColAddress);
        auto size = getCellUserdata(row, ColSize);
        if(end == 0)
        {
            start = base;
        }
        else if(end != base)
        {
            QMessageBox::critical(this, tr("Error"), tr("Dumping non-consecutive memory ranges is not supported!"));
            return;
        }
        end = base + size;
    }

    auto modname = mainModuleName();
    if(!modname.isEmpty())
        modname += '_';
    QString defaultFile = QString("%1/%2%3.bin").arg(QDir::currentPath(), modname, getSelectionText());
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Memory Region"), defaultFile, tr("Binary files (*.bin);;All files (*.*)"));

    if(fileName.length())
    {
        fileName = QDir::toNativeSeparators(fileName);
        DbgCmdExec(QString("savedata \"%1\",%2,%3").arg(fileName, ToPtrString(start), ToHexString(end - start)));
    }
}

void MemoryMapView::loadMemory()
{
    auto modname = mainModuleName();
    if(!modname.isEmpty())
        modname += '_';
    auto addr = getSelectionText();
    QString defaultFile = QString("%1/%2%3.bin").arg(QDir::currentPath(), modname, addr);
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Memory Region"), defaultFile, tr("Binary files (*.bin);;All files (*.*)"));

    if(fileName.length())
    {
        fileName = QDir::toNativeSeparators(fileName);
        //TODO: loaddata command (Does ODbgScript support that?)
        DbgCmdExec(QString("savedata \"%1\",%2,%3").arg(fileName, addr, getCellContent(getInitialSelection(), ColSize)));
    }
}

void MemoryMapView::selectAddress(duint va)
{
    auto base = DbgMemFindBaseAddr(va, nullptr);
    if(base)
    {
        auto rows = getRowCount();
        for(dsint row = 0; row < rows; row++)
            if(getCellUserdata(row, ColAddress) == base)
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
    mGoto->setInitialExpression(ToPtrString(getSelectionAddr()));
    if(mGoto->exec() == QDialog::Accepted)
    {
        selectAddress(DbgValFromString(mGoto->expressionText.toUtf8().constData()));
    }
}

void MemoryMapView::addVirtualModSlot()
{
    auto base = getSelectionAddr();
    auto size = getCellUserdata(getInitialSelection(), ColSize);
    VirtualModDialog mDialog(this);
    mDialog.setData("", base, size);
    if(mDialog.exec() != QDialog::Accepted)
        return;
    QString modname;
    if(!mDialog.getData(modname, base, size))
        return;
    DbgCmdExec(QString("virtualmod \"%1\", %2, %3").arg(modname).arg(ToHexString(base)).arg(ToHexString(size)));
}

void MemoryMapView::findReferencesSlot()
{
    auto base = getSelectionAddr();
    auto size = getCellUserdata(getInitialSelection(), ColSize);
    DbgCmdExec(QString("reffindrange %1, %2, dis.sel()").arg(ToPtrString(base)).arg(ToPtrString(base + size)));
    emit showReferences();
}

void MemoryMapView::selectionGetSlot(SELECTIONDATA* selection)
{
    auto sel = getSelection();
    selection->start = getCellUserdata(sel.front(), ColAddress);
    selection->end = getCellUserdata(sel.back(), ColAddress) + getCellUserdata(sel.back(), ColSize) - 1;
    Bridge::getBridge()->setResult(BridgeResult::SelectionGet, 1);
}

void MemoryMapView::disassembleAtSlot(dsint va, dsint cip)
{
    Q_UNUSED(va)
    mCipBase = DbgMemFindBaseAddr(cip, nullptr);;
}

void MemoryMapView::commentSlot()
{
    duint wVA = getSelectionAddr();
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

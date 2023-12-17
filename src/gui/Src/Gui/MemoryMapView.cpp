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

    connect(Bridge::getBridge(), SIGNAL(updateMemory()), this, SLOT(refreshMapSlot()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(stateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(selectInMemoryMap(duint)), this, SLOT(selectAddressSlot(duint)));
    connect(Bridge::getBridge(), SIGNAL(selectionMemmapGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionMemmapSet(const SELECTIONDATA*)), this, SLOT(selectionSetSlot(const SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(duint, duint)), this, SLOT(disassembleAtSlot(duint, duint)));
    connect(Bridge::getBridge(), SIGNAL(focusMemmap()), this, SLOT(setFocus()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(sortHappenedSignal()), this, SLOT(sortHappenedSlot()));
    connect(this, SIGNAL(selectionChanged(duint)), this, SLOT(selectionChangedSlot(duint)));

    setupContextMenu();
}

void MemoryMapView::sortHappenedSlot()
{
    selectAddressRange(mSelectedAddressRange);
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
    connect(mPageMemoryRights, SIGNAL(triggered()), this, SLOT(pageMemoryRightsSlot()));

    //Switch View
    mSwitchView = new QAction(DIcon("change-view"), "", this);
    setSwitchViewName();
    connect(mSwitchView, SIGNAL(triggered()), this, SLOT(switchViewSlot()));

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
    connect(mDumpMemory, SIGNAL(triggered()), this, SLOT(dumpMemorySlot()));

    //Load
    mLoadMemory = new QAction(DIcon(""), tr("&Overwrite with Data from File"), this);
    connect(mLoadMemory, SIGNAL(triggered()), this, SLOT(loadMemorySlot()));

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

void MemoryMapView::selectionChangedSlot(duint index)
{
    mSelectedAddressRange = getAddressRangeFromSelection(getSelection());
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

    QMenu menu(this); //create context menu
    menu.addAction(mFollowDisassembly);
    menu.addAction(mFollowDump);

    if(DbgFunctions()->ModBaseFromAddr(selectedAddr))
        menu.addAction(mFollowSymbols);

    menu.addAction(mDumpMemory);
    //menu.addAction(mLoadMemory); //TODO:loaddata command
    menu.addAction(mComment);
    menu.addAction(mFindPattern);
    menu.addAction(mSwitchView);
    menu.addAction(mReferences);
    menu.addSeparator();
    menu.addAction(mMemoryAllocate);
    menu.addAction(mMemoryFree);
    menu.addAction(mAddVirtualMod);
    menu.addMenu(mGotoMenu);
    menu.addSeparator();
    menu.addAction(mPageMemoryRights);
    menu.addSeparator();
    menu.addMenu(mBreakpointMenu);
    menu.addSeparator();
    DbgMenuPrepare(GUI_MEMMAP_MENU);
    menu.addActions(mPluginMenu->actions());
    QMenu copyMenu(tr("&Copy"), this);
    copyMenu.setIcon(DIcon("copy"));
    setupCopyMenu(&copyMenu);
    if(copyMenu.actions().length())
    {
        menu.addSeparator();
        menu.addMenu(&copyMenu);
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

    menu.exec(mapToGlobal(pos)); //execute context menu
}

QString MemoryMapView::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    if(col == 0) //address
    {
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
        else if(isSelected(row) == true)
            painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));

        if(backgroundColor.alpha())
            painter->fillRect(QRect(x, y, w - 1, h), QBrush(backgroundColor));
        painter->setPen(color);
        QString str = getCellContent(row, col);
        painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, str);
        return QString();
    }
    else if(col == ColPageInfo) //info
    {
        QString str = StdIconTable::paintContent(painter, row, col, x, y, w, h);
        auto addr = getCellUserdata(row, ColAddress);
        if(str.contains(" \""))
        {
            auto idx = str.indexOf(" \"");
            auto pre = str.mid(0, idx);
            auto post = str.mid(idx);
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
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, str);
            return QString();
        }
    }
    else if(col == ColCurProtect) //CPROT
    {
        QString str = StdIconTable::paintContent(painter, row, col, x, y, w, h);;
        if(!ConfigBool("Engine", "ListAllPages"))
        {
            painter->setPen(ConfigColor("MemoryMapSectionTextColor"));
            painter->drawText(QRect(x + 4, y, getColumnWidth(col) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, str);
            return QString();
        }
    }
    return StdIconTable::paintContent(painter, row, col, x, y, w, h);
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
    connect(action, SIGNAL(triggered()), this, SLOT(execCommandSlot()));
    return action;
}

/**
 * @brief MemoryMapView::execCommandSlot execute command slot for menus.
 */
void MemoryMapView::execCommandSlot()
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

static QString getProtectionString(DWORD protect)
{
    //reserved pages don't have a protection (https://goo.gl/Izkk0c)
    if(protect == 0)
    {
        return QString();
    }

#define meme(prot, str) (prot & PAGE_GUARD) ? QStringLiteral(str QT_UNICODE_LITERAL("G")) : QStringLiteral(str QT_UNICODE_LITERAL("-"))

    switch(protect & 0xFF)
    {
    case PAGE_NOACCESS:
        return meme(protect, "----");
    case PAGE_READONLY:
        return meme(protect, "-R--");
    case PAGE_READWRITE:
        return meme(protect, "-RW-");
    case PAGE_WRITECOPY:
        return meme(protect, "-RWC");
    case PAGE_EXECUTE:
        return meme(protect, "E---");
    case PAGE_EXECUTE_READ:
        return meme(protect, "ER--");
    case PAGE_EXECUTE_READWRITE:
        return meme(protect, "ERW-");
    case PAGE_EXECUTE_WRITECOPY:
        return meme(protect, "ERWC");
    default:
        return meme(protect, "????");
    }
}


void MemoryMapView::selectAddressRange(const std::pair<duint, duint> & addressRange)
{
    if(addressRange.first == 0 || addressRange.second == 0)
        return;

    const auto savedAddressRange = addressRange;

    QList<duint> selectedRows{};

    for(duint i = mSort.ascending ? 0 : (getRowCount() - 1); i < getRowCount();  mSort.ascending ? i++ : i--)
    {
        auto currRowAddress = getCellUserdata(i, ColAddress);
        auto currRowSize = getCellUserdata(i, ColSize);
        std::pair<duint, duint> currAddressRange{currRowAddress, currRowAddress + currRowSize};

        if(currAddressRange.second != 0)
            currAddressRange.second--;

        if((currAddressRange.first >= savedAddressRange.first && savedAddressRange.first <= currAddressRange.second) &&
                (currAddressRange.second <= savedAddressRange.second && savedAddressRange.second >= currAddressRange.first))
        {
            selectedRows.append(i);
        }
        else if(!selectedRows.empty())
        {
            break;
        }
    }

    if(selectedRows.empty())
        return;

    SelectionData toSet{};

    if(mSort.ascending)
    {
        toSet.firstSelectedIndex = selectedRows.at(0);
        toSet.toIndex = *(selectedRows.cend() - 1);
    }
    else
    {
        toSet.firstSelectedIndex = *(selectedRows.cend() - 1);
        toSet.toIndex = selectedRows.at(0);
    }

    toSet.fromIndex = toSet.firstSelectedIndex;
    tryEmitAddressSelectionChange(toSet, savedAddressRange);
}

void MemoryMapView::tryEmitAddressSelectionChange(const SelectionData & newSelection, const std::pair<duint, duint> & basedUpon)
{
    if(basedUpon.first == mSelectedAddressRange.first && basedUpon.second == mSelectedAddressRange.second)
    {
        if(newSelection.firstSelectedIndex != mSelection.firstSelectedIndex ||
                newSelection.fromIndex != mSelection.fromIndex ||
                newSelection.toIndex != mSelection.toIndex)
        {
            mSelection = newSelection;
            emit selectionChanged(newSelection.firstSelectedIndex);
        }
    }
}

std::pair<duint, duint> MemoryMapView::getAddressRangeFromSelection(QList<duint> & selection)
{
    if(selection.empty())
        return {};

    std::map<duint, duint> entries;

    for(duint i : selection)
    {
        entries.emplace(
            std::pair<duint, duint>
            (getCellUserdata(i, ColAddress), i));
    }

    duint startAddress = entries.cbegin()->first;

    auto lastValidElement = --entries.cend();
    duint lastAddressRow = lastValidElement->second;
    duint lastAddressSize = getCellUserdata(lastAddressRow, ColSize);

    duint lastAddress = lastValidElement->first + lastAddressSize;

    if(lastAddress != 0) // watchout for 0
        lastAddress--;   // decrementing by one to make it [addr, addr + size - 1] instead of [addr, addr + size)

    return {startAddress, lastAddress};
}

void MemoryMapView::refreshMapSlot()
{
    MEMMAP memoryMap = {};
    DbgMemMap(&memoryMap);

    setRowCount(memoryMap.count);

    auto strUser = tr("User");
    auto strSystem = tr("System");

    for(int i = 0; i < memoryMap.count; i++)
    {
        const auto & mbi = memoryMap.page[i].mbi;

        // Base address
        setCellContent(i, ColAddress, ToPtrString((duint)mbi.BaseAddress));
        setCellUserdata(i, ColAddress, (duint)mbi.BaseAddress);

        // Size
        setCellContent(i, ColSize, ToPtrString((duint)mbi.RegionSize));
        setCellUserdata(i, ColSize, (duint)mbi.RegionSize);

        // Party
        int party = DbgFunctions()->ModGetParty((duint)mbi.BaseAddress);
        switch(party)
        {
        case mod_user:
            setCellContent(i, ColParty, strUser);
            setRowIcon(i, DIcon("markasuser"));
            break;
        case mod_system:
            setCellContent(i, ColParty, strSystem);
            setRowIcon(i, DIcon("markassystem"));
            break;
        default:
            setCellContent(i, ColParty, QString::number(party));
            setRowIcon(i, DIcon("markasparty"));
            break;
        }

        // Information
        auto info = memoryMap.page[i].info;
        setCellContent(i, ColPageInfo, info);

        // Content, TODO: proper section content analysis in dbg/memory.cpp:MemUpdateMap
        QString content;
        char comment_text[MAX_COMMENT_SIZE];
        if(DbgFunctions()->GetUserComment((duint)mbi.BaseAddress, comment_text)) // user comment present
            content = comment_text;
        else if(strncmp(info, ".bss", 4) == 0)
            content = tr("Uninitialized data");
        else if(strncmp(info, ".data", 5) == 0)
            content = tr("Initialized data");
        else if(strncmp(info, ".edata", 6) == 0)
            content = tr("Export tables");
        else if(strncmp(info, ".idata", 6) == 0)
            content = tr("Import tables");
        else if(strncmp(info, ".pdata", 6) == 0)
            content = tr("Exception information");
        else if(strncmp(info, ".rdata", 6) == 0)
            content = tr("Read-only initialized data");
        else if(strncmp(info, ".reloc", 6) == 0)
            content = tr("Base relocations");
        else if(strncmp(info, ".rsrc", 5) == 0)
            content = tr("Resources");
        else if(strncmp(info, ".text", 5) == 0)
            content = tr("Executable code");
        else if(strncmp(info, ".tls", 4) == 0)
            content = tr("Thread-local storage");
        else if(strncmp(info, ".xdata", 6) == 0)
            content = tr("Exception information");
        setCellContent(i, ColContent, std::move(content));

        // Type
        QString type;
        switch(mbi.Type)
        {
        case MEM_IMAGE:
            type = QStringLiteral("IMG");
            break;
        case MEM_MAPPED:
            type = QStringLiteral("MAP");
            break;
        case MEM_PRIVATE:
            type = QStringLiteral("PRV");
            break;
        default:
            type = QStringLiteral("N/A");
            break;
        }
        setCellContent(i, ColAllocation, std::move(type));

        // current access protection
        setCellContent(i, ColCurProtect, getProtectionString(mbi.Protect));

        // allocation protection
        setCellContent(i, ColAllocProtect, getProtectionString(mbi.AllocationProtect));
    }
    if(memoryMap.page != 0)
        BridgeFree(memoryMap.page);

    reloadData(); //refresh memory map

    selectAddressRange(mSelectedAddressRange);
}

void MemoryMapView::stateChangedSlot(DBGSTATE state)
{
    if(state == paused)
        refreshMapSlot();
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
        QString addrText = getCellContent(i, ColAddress);
        duint selectedAddr = getSelectionAddr();
        if((DbgGetBpxTypeAt(selectedAddr) & bp_memory) == bp_memory) //memory breakpoint set
            DbgCmdExec(QString("bpmc ") + addrText);
        else
            DbgCmdExec(QString("bpm %1, 0, x").arg(addrText));
    }
}

void MemoryMapView::pageMemoryRightsSlot()
{
    PageMemoryRights PageMemoryRightsDialog(this);
    connect(&PageMemoryRightsDialog, SIGNAL(refreshMemoryMap()), this, SLOT(refreshMapSlot()));
    duint addr = getSelectionAddr();
    duint size = getCellUserdata(getInitialSelection(), ColSize);
    PageMemoryRightsDialog.RunAddrSize(addr, size, getCellContent(getInitialSelection(), ColCurProtect));
}

void MemoryMapView::switchViewSlot()
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
    hexEdit.isDataCopiable(false);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;

    entireBlockEnabled = hexEdit.entireBlock();
    BridgeSettingSetUint("Gui", "MemoryMapEntireBlock", entireBlockEnabled);
    if(entireBlockEnabled)
    {
        DbgCmdExec(QString("findallmem 0, %2").arg(hexEdit.mHexEdit->pattern()));
    }
    else
    {
        QList<duint> selection = getSelection();
        if(selection.isEmpty())
            return;
        duint addrFirst = getCellUserdata(selection.first(), ColAddress);
        duint addrLast = getCellUserdata(selection.last(), ColAddress);
        duint size = getCellUserdata(selection.last(), ColSize);
        DbgCmdExec(QString("findallmem %1, %2, %3").arg(ToPtrString(addrFirst)).arg(hexEdit.mHexEdit->pattern()).arg(ToHexString(addrLast - addrFirst + size)));
    }
    emit showReferences();
}

void MemoryMapView::dumpMemorySlot()
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

void MemoryMapView::loadMemorySlot()
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

void MemoryMapView::selectAddressSlot(duint va)
{
    auto base = DbgMemFindBaseAddr(va, nullptr);
    if(base)
    {
        auto rows = getRowCount();
        for(duint row = 0; row < rows; row++)
        {
            if(getCellUserdata(row, ColAddress) == base)
            {
                scrollSelect(row);
                reloadData();
                return;
            }
        }
    }
    SimpleErrorBox(this, tr("Error"), tr("Address %0 not found in memory map...").arg(ToPtrString(va)));
}

void MemoryMapView::gotoOriginSlot()
{
    selectAddressSlot(mCipBase);
}

void MemoryMapView::gotoExpressionSlot()
{
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->setWindowTitle(tr("Enter the address to find..."));
    mGoto->setInitialExpression(ToPtrString(getSelectionAddr()));
    if(mGoto->exec() == QDialog::Accepted)
    {
        selectAddressSlot(DbgValFromString(mGoto->expressionText.toUtf8().constData()));
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

void MemoryMapView::selectionSetSlot(const SELECTIONDATA* selection)
{
    const auto rowCount = getRowCount();
    if(rowCount == 0)
    {
        Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 0);
        return;
    }

    const auto badIndex = (duint) - 1;
    duint firstIdx = badIndex;
    duint lastIdx = badIndex;

    for(duint row = 0; row < rowCount; row++)
    {
        const duint addrStart = getCellUserdata(row, ColAddress);
        if(addrStart < selection->start)
            continue;

        const duint addrEnd = addrStart + getCellUserdata(row, ColSize);
        if(addrEnd > selection->end)
            break;

        if(firstIdx == badIndex)
            firstIdx = row;

        lastIdx = row;
    }

    if(firstIdx == badIndex || lastIdx == badIndex)
    {
        Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 0);
        return;
    }

    setSingleSelection(firstIdx);
    expandSelectionUpTo(lastIdx);
    reloadData();
    Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 1);
}

void MemoryMapView::disassembleAtSlot(duint va, duint cip)
{
    Q_UNUSED(va)
    mCipBase = DbgMemFindBaseAddr(cip, nullptr);;
}

void MemoryMapView::commentSlot()
{
    duint va = getSelectionAddr();
    LineEditDialog mLineEdit(this);
    QString addrText = ToPtrString(va);
    char comment_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetCommentAt((duint)va, comment_text))
    {
        if(comment_text[0] == '\1') //automatic comment
            mLineEdit.setText(QString(comment_text + 1));
        else
            mLineEdit.setText(QString(comment_text));
    }
    mLineEdit.setWindowTitle(tr("Add comment at ") + addrText);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    if(!DbgSetCommentAt(va, mLineEdit.editText.replace('\r', "").replace('\n', "").toUtf8().constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetCommentAt failed!"));

    GuiUpdateMemoryView();
}

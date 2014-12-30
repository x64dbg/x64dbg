#include "BreakpointsView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "Breakpoints.h"

BreakpointsView::BreakpointsView(QWidget* parent) : QWidget(parent)
{
    // Software
    mSoftBPTable = new StdTable(this);
    int wCharWidth = mSoftBPTable->getCharWidth();
    mSoftBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 2 * sizeof(uint_t), "Software", false, "Address");
    mSoftBPTable->addColumnAt(8 + wCharWidth * 32, "Name", false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 32, "Module/Label", false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 8, "State", false);
    mSoftBPTable->addColumnAt(wCharWidth * 10, "Comment", false);

    // Hardware
    mHardBPTable = new StdTable(this);
    mHardBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mHardBPTable->addColumnAt(8 + wCharWidth * 2 * sizeof(uint_t), "Hardware", false, "Address");
    mHardBPTable->addColumnAt(8 + wCharWidth * 32, "Name", false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 32, "Module/Label", false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 8, "State", false);
    mHardBPTable->addColumnAt(wCharWidth * 10, "Comment", false);

    // Memory
    mMemBPTable = new StdTable(this);
    mMemBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mMemBPTable->addColumnAt(8 + wCharWidth * 2 * sizeof(uint_t), "Memory", false, "Address");
    mMemBPTable->addColumnAt(8 + wCharWidth * 32, "Name", false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 32, "Module/Label", false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 8, "State", false);
    mMemBPTable->addColumnAt(wCharWidth * 10, "Comment", false);

    // Splitter
    mSplitter = new QSplitter(this);
    mSplitter->setOrientation(Qt::Vertical);
    mSplitter->addWidget(mSoftBPTable);
    mSplitter->addWidget(mHardBPTable);
    mSplitter->addWidget(mMemBPTable);

    // Layout
    mVertLayout = new QVBoxLayout;
    mVertLayout->setSpacing(0);
    mVertLayout->setContentsMargins(0, 0, 0, 0);
    mVertLayout->addWidget(mSplitter);
    this->setLayout(mVertLayout);

    // Create the action list for the right click context menu
    setupHardBPRightClickContextMenu();
    setupSoftBPRightClickContextMenu();
    setupMemBPRightClickContextMenu();

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));

    // Signals/Slots
    connect(Bridge::getBridge(), SIGNAL(updateBreakpoints()), this, SLOT(reloadData()));
    connect(mHardBPTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(hardwareBPContextMenuSlot(const QPoint &)));
    connect(mHardBPTable, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickHardwareSlot()));
    connect(mSoftBPTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(softwareBPContextMenuSlot(const QPoint &)));
    connect(mSoftBPTable, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickSoftwareSlot()));
    connect(mMemBPTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(memoryBPContextMenuSlot(const QPoint &)));
    connect(mMemBPTable, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickMemorySlot()));
}


void BreakpointsView::reloadData()
{
    BPMAP wBPList;
    int wI;

    // Hardware
    DbgGetBpList(bp_hardware, &wBPList);
    mHardBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        QString addr_text = QString("%1").arg(wBPList.bp[wI].addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        mHardBPTable->setCellContent(wI, 0, addr_text);
        mHardBPTable->setCellContent(wI, 1, QString(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text = "<" + QString(wBPList.bp[wI].mod) + "." + QString(label) + ">";
        else
            label_text = QString(wBPList.bp[wI].mod);
        mHardBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mHardBPTable->setCellContent(wI, 3, "Inactive");
        else if(wBPList.bp[wI].enabled == true)
            mHardBPTable->setCellContent(wI, 3, "Enabled");
        else
            mHardBPTable->setCellContent(wI, 3, "Disabled");

        char comment[MAX_COMMENT_SIZE] = "";
        if(DbgGetCommentAt(wBPList.bp[wI].addr, comment))
        {
            if(comment[0] == '\1') //automatic comment
                mHardBPTable->setCellContent(wI, 4, QString(comment + 1));
            else
                mHardBPTable->setCellContent(wI, 4, comment);
        }
    }
    mHardBPTable->reloadData();
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    // Software
    DbgGetBpList(bp_normal, &wBPList);
    mSoftBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        QString addr_text = QString("%1").arg(wBPList.bp[wI].addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        mSoftBPTable->setCellContent(wI, 0, addr_text);
        mSoftBPTable->setCellContent(wI, 1, QString(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text = "<" + QString(wBPList.bp[wI].mod) + "." + QString(label) + ">";
        else
            label_text = QString(wBPList.bp[wI].mod);
        mSoftBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mSoftBPTable->setCellContent(wI, 3, "Inactive");
        else if(wBPList.bp[wI].enabled == true)
            mSoftBPTable->setCellContent(wI, 3, "Enabled");
        else
            mSoftBPTable->setCellContent(wI, 3, "Disabled");

        char comment[MAX_COMMENT_SIZE] = "";
        if(DbgGetCommentAt(wBPList.bp[wI].addr, comment))
        {
            if(comment[0] == '\1') //automatic comment
                mSoftBPTable->setCellContent(wI, 4, QString(comment + 1));
            else
                mSoftBPTable->setCellContent(wI, 4, comment);
        }
    }
    mSoftBPTable->reloadData();
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    // Memory
    DbgGetBpList(bp_memory, &wBPList);
    mMemBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        QString addr_text = QString("%1").arg(wBPList.bp[wI].addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        mMemBPTable->setCellContent(wI, 0, addr_text);
        mMemBPTable->setCellContent(wI, 1, QString(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text = "<" + QString(wBPList.bp[wI].mod) + "." + QString(label) + ">";
        else
            label_text = QString(wBPList.bp[wI].mod);
        mMemBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mMemBPTable->setCellContent(wI, 3, "Inactive");
        else if(wBPList.bp[wI].enabled == true)
            mMemBPTable->setCellContent(wI, 3, "Enabled");
        else
            mMemBPTable->setCellContent(wI, 3, "Disabled");

        char comment[MAX_COMMENT_SIZE] = "";
        if(DbgGetCommentAt(wBPList.bp[wI].addr, comment))
        {
            if(comment[0] == '\1') //automatic comment
                mMemBPTable->setCellContent(wI, 4, QString(comment + 1));
            else
                mMemBPTable->setCellContent(wI, 4, comment);
        }
    }
    mMemBPTable->reloadData();
    if(wBPList.count)
        BridgeFree(wBPList.bp);
}


/************************************************************************************
                         Hardware Context Menu Management
************************************************************************************/
void BreakpointsView::setupHardBPRightClickContextMenu()
{
    // Remove
    mHardBPRemoveAction = new QAction("Remove", this);
    mHardBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mHardBPTable->addAction(mHardBPRemoveAction);
    connect(mHardBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeHardBPActionSlot()));

    // Remove All
    mHardBPRemoveAllAction = new QAction("Remove All", this);
    connect(mHardBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllHardBPActionSlot()));

    // Enable/Disable
    mHardBPEnableDisableAction = new QAction("Enable", this);
    mHardBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mHardBPTable->addAction(mHardBPEnableDisableAction);
    connect(mHardBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableHardBPActionSlot()));
}

void BreakpointsView::refreshShortcutsSlot()
{
    mHardBPRemoveAction->setShortcut(ConfigShortcut("ActionDeleteBreakpoint"));
    mHardBPEnableDisableAction->setShortcut(ConfigShortcut("ActionEnableDisableBreakpoint"));

    mSoftBPRemoveAction->setShortcut(ConfigShortcut("ActionDeleteBreakpoint"));
    mSoftBPEnableDisableAction->setShortcut(ConfigShortcut("ActionEnableDisableBreakpoint"));

    mMemBPRemoveAction->setShortcut(ConfigShortcut("ActionDeleteBreakpoint"));
    mMemBPEnableDisableAction->setShortcut(ConfigShortcut("ActionEnableDisableBreakpoint"));
}

void BreakpointsView::hardwareBPContextMenuSlot(const QPoint & pos)
{
    StdTable* table = mHardBPTable;
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        QMenu* wMenu = new QMenu(this);
        uint_t wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
        BPMAP wBPList;

        // Remove
        wMenu->addAction(mHardBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_hardware, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(wBPList.bp[wI].addr == wVA)
            {
                if(wBPList.bp[wI].active == false)
                {
                    // Nothing
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mHardBPEnableDisableAction->setText("Disable");
                    wMenu->addAction(mHardBPEnableDisableAction);
                }
                else
                {
                    mHardBPEnableDisableAction->setText("Enable");
                    wMenu->addAction(mHardBPEnableDisableAction);
                }
            }
        }
        if(wBPList.count)
            BridgeFree(wBPList.bp);

        // Separator
        wMenu->addSeparator();

        // Remove All
        wMenu->addAction(mHardBPRemoveAllAction);

        //Copy
        QMenu wCopyMenu("&Copy", this);
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }

        wMenu->exec(table->mapToGlobal(pos));
    }
}

void BreakpointsView::removeHardBPActionSlot()
{
    StdTable* table = mHardBPTable;
    uint_t wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
    Breakpoints::removeBP(bp_hardware, wVA);
}

void BreakpointsView::removeAllHardBPActionSlot()
{
    DbgCmdExec("bphwc");
}

void BreakpointsView::enableDisableHardBPActionSlot()
{
    StdTable* table = mHardBPTable;
    Breakpoints::toggleBPByDisabling(bp_hardware, table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16));
    int_t sel = table->getInitialSelection();
    if(sel + 1 < table->getRowCount())
        table->setSingleSelection(sel + 1);
}

void BreakpointsView::doubleClickHardwareSlot()
{
    StdTable* table = mHardBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    emit showCpu();
}


/************************************************************************************
                         Software Context Menu Management
************************************************************************************/
void BreakpointsView::setupSoftBPRightClickContextMenu()
{
    // Remove
    mSoftBPRemoveAction = new QAction("Remove", this);
    mSoftBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mSoftBPTable->addAction(mSoftBPRemoveAction);
    connect(mSoftBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeSoftBPActionSlot()));

    // Remove All
    mSoftBPRemoveAllAction = new QAction("Remove All", this);
    connect(mSoftBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllSoftBPActionSlot()));

    // Enable/Disable
    mSoftBPEnableDisableAction = new QAction("Enable", this);
    mSoftBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mSoftBPTable->addAction(mSoftBPEnableDisableAction);
    connect(mSoftBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableSoftBPActionSlot()));
}

void BreakpointsView::softwareBPContextMenuSlot(const QPoint & pos)
{
    StdTable* table = mSoftBPTable;
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        QMenu* wMenu = new QMenu(this);
        uint_t wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
        BPMAP wBPList;

        // Remove
        wMenu->addAction(mSoftBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_normal, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(wBPList.bp[wI].addr == wVA)
            {
                if(wBPList.bp[wI].active == false)
                {
                    // Nothing
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mSoftBPEnableDisableAction->setText("Disable");
                    wMenu->addAction(mSoftBPEnableDisableAction);
                }
                else
                {
                    mSoftBPEnableDisableAction->setText("Enable");
                    wMenu->addAction(mSoftBPEnableDisableAction);
                }
            }
        }
        if(wBPList.count)
            BridgeFree(wBPList.bp);

        // Separator
        wMenu->addSeparator();

        // Remove All
        wMenu->addAction(mSoftBPRemoveAllAction);

        //Copy
        QMenu wCopyMenu("&Copy", this);
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }

        wMenu->exec(table->mapToGlobal(pos));
    }
}

void BreakpointsView::removeSoftBPActionSlot()
{
    StdTable* table = mSoftBPTable;
    uint_t wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
    Breakpoints::removeBP(bp_normal, wVA);
}

void BreakpointsView::removeAllSoftBPActionSlot()
{
    DbgCmdExec("bc");
}

void BreakpointsView::enableDisableSoftBPActionSlot()
{
    StdTable* table = mSoftBPTable;
    Breakpoints::toggleBPByDisabling(bp_normal, table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16));
    int_t sel = table->getInitialSelection();
    if(sel + 1 < table->getRowCount())
        table->setSingleSelection(sel + 1);
}

void BreakpointsView::doubleClickSoftwareSlot()
{
    StdTable* table = mSoftBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    emit showCpu();
}


/************************************************************************************
                         Memory Context Menu Management
************************************************************************************/
void BreakpointsView::setupMemBPRightClickContextMenu()
{
    // Remove
    mMemBPRemoveAction = new QAction("Remove", this);
    mMemBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mMemBPTable->addAction(mMemBPRemoveAction);
    connect(mMemBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeMemBPActionSlot()));

    // Remove All
    mMemBPRemoveAllAction = new QAction("Remove All", this);
    connect(mMemBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllMemBPActionSlot()));

    // Enable/Disable
    mMemBPEnableDisableAction = new QAction("Enable", this);
    mMemBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mMemBPTable->addAction(mMemBPEnableDisableAction);
    connect(mMemBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableMemBPActionSlot()));
}

void BreakpointsView::memoryBPContextMenuSlot(const QPoint & pos)
{
    StdTable* table = mMemBPTable;
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        QMenu* wMenu = new QMenu(this);
        uint_t wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
        BPMAP wBPList;

        // Remove
        wMenu->addAction(mMemBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_memory, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(wBPList.bp[wI].addr == wVA)
            {
                if(wBPList.bp[wI].active == false)
                {
                    // Nothing
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mMemBPEnableDisableAction->setText("Disable");
                    wMenu->addAction(mMemBPEnableDisableAction);
                }
                else
                {
                    mMemBPEnableDisableAction->setText("Enable");
                    wMenu->addAction(mMemBPEnableDisableAction);
                }
            }
        }
        if(wBPList.count)
            BridgeFree(wBPList.bp);

        // Separator
        wMenu->addSeparator();

        // Remove All
        wMenu->addAction(mMemBPRemoveAllAction);

        //Copy
        QMenu wCopyMenu("&Copy", this);
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu->addSeparator();
            wMenu->addMenu(&wCopyMenu);
        }

        wMenu->exec(table->mapToGlobal(pos));
    }
}

void BreakpointsView::removeMemBPActionSlot()
{
    StdTable* table = mMemBPTable;
    uint_t wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
    Breakpoints::removeBP(bp_memory, wVA);
}

void BreakpointsView::removeAllMemBPActionSlot()
{
    DbgCmdExec("bpmc");
}

void BreakpointsView::enableDisableMemBPActionSlot()
{
    StdTable* table = mMemBPTable;
    Breakpoints::toggleBPByDisabling(bp_memory, table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16));
    int_t sel = table->getInitialSelection();
    if(sel + 1 < table->getRowCount())
        table->setSingleSelection(sel + 1);
}

void BreakpointsView::doubleClickMemorySlot()
{
    StdTable* table = mMemBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    emit showCpu();
}

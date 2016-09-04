#include <QVBoxLayout>
#include "BreakpointsView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "Breakpoints.h"
#include "LineEditDialog.h"
#include "StdTable.h"
#include "LabeledSplitter.h"

BreakpointsView::BreakpointsView(QWidget* parent) : QWidget(parent)
{
    // Software
    mSoftBPTable = new StdTable(this);
    int wCharWidth = mSoftBPTable->getCharWidth();
    mSoftBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 2 * sizeof(duint), tr("Software"), false, tr("Address"));
    mSoftBPTable->addColumnAt(8 + wCharWidth * 32, tr("Name"), false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 32, tr("Module/Label"), false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 8, tr("State"), false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 10, tr("Hit count"), false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 32, tr("Log text"), false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 32, tr("Condition"), false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 2, tr("Fast resume"), false);
    mSoftBPTable->addColumnAt(8 + wCharWidth * 16, tr("Command on hit"), false);
    mSoftBPTable->addColumnAt(wCharWidth * 10, tr("Comment"), false);
    mSoftBPTable->loadColumnFromConfig("SoftwareBreakpoint");

    // Hardware
    mHardBPTable = new StdTable(this);
    mHardBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mHardBPTable->addColumnAt(8 + wCharWidth * 2 * sizeof(duint), tr("Hardware"), false, tr("Address"));
    mHardBPTable->addColumnAt(8 + wCharWidth * 32, tr("Name"), false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 32, tr("Module/Label"), false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 8, tr("State"), false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 10, tr("Hit count"), false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 32, tr("Log text"), false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 32, tr("Condition"), false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 2, tr("Fast resume"), false);
    mHardBPTable->addColumnAt(8 + wCharWidth * 16, tr("Command on hit"), false);
    mHardBPTable->addColumnAt(wCharWidth * 10, tr("Comment"), false);
    mHardBPTable->loadColumnFromConfig("HardwareBreakpoint");

    // Memory
    mMemBPTable = new StdTable(this);
    mMemBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mMemBPTable->addColumnAt(8 + wCharWidth * 2 * sizeof(duint), tr("Memory"), false, tr("Address"));
    mMemBPTable->addColumnAt(8 + wCharWidth * 32, tr("Name"), false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 32, tr("Module/Label"), false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 8, tr("State"), false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 10, tr("Hit count"), false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 32, tr("Log text"), false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 32, tr("Condition"), false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 2, tr("Fast resume"), false);
    mMemBPTable->addColumnAt(8 + wCharWidth * 16, tr("Command on hit"), false);
    mMemBPTable->addColumnAt(wCharWidth * 10, tr("Comment"), false);
    mMemBPTable->loadColumnFromConfig("MemoryBreakpoint");

    // DLL
    mDLLBPTable = new StdTable(this);
    mDLLBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mDLLBPTable->addColumnAt(8 + wCharWidth * 32, tr("Name"), false);
    mDLLBPTable->addColumnAt(8 + wCharWidth * 32, tr("Module"), false);
    mDLLBPTable->addColumnAt(8 + wCharWidth * 8, tr("State"), false);
    mDLLBPTable->addColumnAt(8 + wCharWidth * 10, tr("Hit count"), false);
    mDLLBPTable->addColumnAt(8 + wCharWidth * 32, tr("Log text"), false);
    mDLLBPTable->addColumnAt(8 + wCharWidth * 32, tr("Condition"), false);
    mDLLBPTable->addColumnAt(8 + wCharWidth * 2, tr("Fast resume"), false);
    mDLLBPTable->addColumnAt(8 + wCharWidth * 16, tr("Command on hit"), false);
    mDLLBPTable->loadColumnFromConfig("DLLBreakpoint");

    // Splitter
    mSplitter = new LabeledSplitter(this);
    mSplitter->addWidget(mSoftBPTable, tr("Software breakpoint"));
    mSplitter->addWidget(mHardBPTable, tr("Hardware breakpoint"));
    mSplitter->addWidget(mMemBPTable, tr("Memory breakpoint"));
    mSplitter->addWidget(mDLLBPTable, tr("DLL breakpoint"));
    mSplitter->collapseLowerTabs();

    // Layout
    mVertLayout = new QVBoxLayout;
    mVertLayout->setSpacing(0);
    mVertLayout->setContentsMargins(0, 0, 0, 0);
    mVertLayout->addWidget(mSplitter);
    this->setLayout(mVertLayout);

    // Create the action list for the right click context menu
    setupRightClickContextMenu();

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));

    // Signals/Slots
    connect(Bridge::getBridge(), SIGNAL(updateBreakpoints()), this, SLOT(reloadData()));
    connect(mHardBPTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(hardwareBPContextMenuSlot(const QPoint &)));
    connect(mHardBPTable, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickHardwareSlot()));
    connect(mHardBPTable, SIGNAL(enterPressedSignal()), this, SLOT(doubleClickHardwareSlot()));
    connect(mHardBPTable, SIGNAL(selectionChangedSignal(int)), this, SLOT(selectionChangedHardwareSlot()));
    connect(mSoftBPTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(softwareBPContextMenuSlot(const QPoint &)));
    connect(mSoftBPTable, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickSoftwareSlot()));
    connect(mSoftBPTable, SIGNAL(enterPressedSignal()), this, SLOT(doubleClickSoftwareSlot()));
    connect(mSoftBPTable, SIGNAL(selectionChangedSignal(int)), this, SLOT(selectionChangedSoftwareSlot()));
    connect(mMemBPTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(memoryBPContextMenuSlot(const QPoint &)));
    connect(mMemBPTable, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickMemorySlot()));
    connect(mMemBPTable, SIGNAL(enterPressedSignal()), this, SLOT(doubleClickMemorySlot()));
    connect(mMemBPTable, SIGNAL(selectionChangedSignal(int)), this, SLOT(selectionChangedMemorySlot()));
    connect(mDLLBPTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(DLLBPContextMenuSlot(const QPoint &)));
    connect(mDLLBPTable, SIGNAL(selectionChangedSignal(int)), this, SLOT(selectionChangedDLLSlot()));


    mCurrentType = bp_normal;
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
        QString addr_text = QString("%1").arg(wBPList.bp[wI].addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        mHardBPTable->setCellContent(wI, 0, addr_text);
        mHardBPTable->setCellContent(wI, 1, QString::fromUtf8(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text = "<" + QString::fromUtf8(wBPList.bp[wI].mod) + "." + QString::fromUtf8(label) + ">";
        else
            label_text = QString::fromUtf8(wBPList.bp[wI].mod);
        mHardBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mHardBPTable->setCellContent(wI, 3, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mHardBPTable->setCellContent(wI, 3, tr("Enabled"));
        else
            mHardBPTable->setCellContent(wI, 3, tr("Disabled"));

        mHardBPTable->setCellContent(wI, 4, QString("%1").arg(wBPList.bp[wI].hitCount));
        mHardBPTable->setCellContent(wI, 5, QString().fromUtf8(wBPList.bp[wI].logText));
        mHardBPTable->setCellContent(wI, 6, QString().fromUtf8(wBPList.bp[wI].breakCondition));
        mHardBPTable->setCellContent(wI, 7, wBPList.bp[wI].fastResume ? "X" : "");
        mHardBPTable->setCellContent(wI, 8, QString().fromUtf8(wBPList.bp[wI].commandText));

        QString comment;
        if(GetCommentFormat(wBPList.bp[wI].addr, comment))
            mHardBPTable->setCellContent(wI, 9, comment);
        else
            mHardBPTable->setCellContent(wI, 9, "");

    }
    mHardBPTable->reloadData();
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    // Software
    DbgGetBpList(bp_normal, &wBPList);
    mSoftBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        QString addr_text = QString("%1").arg(wBPList.bp[wI].addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        mSoftBPTable->setCellContent(wI, 0, addr_text);
        mSoftBPTable->setCellContent(wI, 1, QString::fromUtf8(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text = "<" + QString::fromUtf8(wBPList.bp[wI].mod) + "." + QString::fromUtf8(label) + ">";
        else
            label_text = QString::fromUtf8(wBPList.bp[wI].mod);
        mSoftBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mSoftBPTable->setCellContent(wI, 3, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mSoftBPTable->setCellContent(wI, 3, tr("Enabled"));
        else
            mSoftBPTable->setCellContent(wI, 3, tr("Disabled"));

        mSoftBPTable->setCellContent(wI, 4, QString("%1").arg(wBPList.bp[wI].hitCount));
        mSoftBPTable->setCellContent(wI, 5, QString().fromUtf8(wBPList.bp[wI].logText));
        mSoftBPTable->setCellContent(wI, 6, QString().fromUtf8(wBPList.bp[wI].breakCondition));
        mSoftBPTable->setCellContent(wI, 7, wBPList.bp[wI].fastResume ? "X" : "");
        mSoftBPTable->setCellContent(wI, 8, QString().fromUtf8(wBPList.bp[wI].commandText));

        QString comment;
        if(GetCommentFormat(wBPList.bp[wI].addr, comment))
            mSoftBPTable->setCellContent(wI, 9, comment);
        else
            mSoftBPTable->setCellContent(wI, 9, "");
    }
    mSoftBPTable->reloadData();
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    // Memory
    DbgGetBpList(bp_memory, &wBPList);
    mMemBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        QString addr_text = QString("%1").arg(wBPList.bp[wI].addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        mMemBPTable->setCellContent(wI, 0, addr_text);
        mMemBPTable->setCellContent(wI, 1, QString::fromUtf8(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text = "<" + QString::fromUtf8(wBPList.bp[wI].mod) + "." + QString::fromUtf8(label) + ">";
        else
            label_text = QString::fromUtf8(wBPList.bp[wI].mod);
        mMemBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mMemBPTable->setCellContent(wI, 3, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mMemBPTable->setCellContent(wI, 3, tr("Enabled"));
        else
            mMemBPTable->setCellContent(wI, 3, tr("Disabled"));

        mMemBPTable->setCellContent(wI, 4, QString("%1").arg(wBPList.bp[wI].hitCount));
        mMemBPTable->setCellContent(wI, 5, QString().fromUtf8(wBPList.bp[wI].logText));
        mMemBPTable->setCellContent(wI, 6, QString().fromUtf8(wBPList.bp[wI].breakCondition));
        mMemBPTable->setCellContent(wI, 7, wBPList.bp[wI].fastResume ? "X" : "");
        mMemBPTable->setCellContent(wI, 8, QString().fromUtf8(wBPList.bp[wI].commandText));

        QString comment;
        if(GetCommentFormat(wBPList.bp[wI].addr, comment))
            mMemBPTable->setCellContent(wI, 9, comment);
        else
            mMemBPTable->setCellContent(wI, 9, "");
    }
    mMemBPTable->reloadData();

    // DLL
    DbgGetBpList(bp_dll, &wBPList);
    mDLLBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        mDLLBPTable->setCellContent(wI, 0, QString::fromUtf8(wBPList.bp[wI].name));
        mDLLBPTable->setCellContent(wI, 1, QString::fromUtf8(wBPList.bp[wI].mod));

        if(wBPList.bp[wI].active == false)
            mDLLBPTable->setCellContent(wI, 2, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mDLLBPTable->setCellContent(wI, 2, tr("Enabled"));
        else
            mDLLBPTable->setCellContent(wI, 2, tr("Disabled"));

        mDLLBPTable->setCellContent(wI, 3, QString("%1").arg(wBPList.bp[wI].hitCount));
        mDLLBPTable->setCellContent(wI, 4, QString().fromUtf8(wBPList.bp[wI].logText));
        mDLLBPTable->setCellContent(wI, 5, QString().fromUtf8(wBPList.bp[wI].breakCondition));
        mDLLBPTable->setCellContent(wI, 6, wBPList.bp[wI].fastResume ? "X" : "");
        mDLLBPTable->setCellContent(wI, 7, QString().fromUtf8(wBPList.bp[wI].commandText));
    }
    mDLLBPTable->reloadData();

    if(wBPList.count)
        BridgeFree(wBPList.bp);
}

void BreakpointsView::setupRightClickContextMenu()
{
    mEditBreakpointAction = new QAction(tr("&Edit"), this);
    addAction(mEditBreakpointAction);
    connect(mEditBreakpointAction, SIGNAL(triggered(bool)), this, SLOT(editBreakpointSlot()));

    setupSoftBPRightClickContextMenu();
    setupHardBPRightClickContextMenu();
    setupMemBPRightClickContextMenu();
    setupDLLBPRightClickContextMenu();
}

/************************************************************************************
                         Hardware Context Menu Management
************************************************************************************/
void BreakpointsView::setupHardBPRightClickContextMenu()
{
    // Remove
    mHardBPRemoveAction = new QAction(tr("Remove"), this);
    mHardBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mHardBPTable->addAction(mHardBPRemoveAction);
    connect(mHardBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeHardBPActionSlot()));

    // Remove All
    mHardBPRemoveAllAction = new QAction(tr("Remove All"), this);
    connect(mHardBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllHardBPActionSlot()));

    // Enable/Disable
    mHardBPEnableDisableAction = new QAction(tr("Enable"), this);
    mHardBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mHardBPTable->addAction(mHardBPEnableDisableAction);
    connect(mHardBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableHardBPActionSlot()));

    // Reset hit count
    mHardBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mHardBPTable->addAction(mHardBPResetHitCountAction);
    connect(mHardBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetHardwareHitCountSlot()));

    // Enable All
    mHardBPEnableAllAction = new QAction(tr("Enable All"), this);
    mHardBPTable->addAction(mHardBPEnableAllAction);
    connect(mHardBPEnableAllAction, SIGNAL(triggered()), this, SLOT(enableAllHardBPActionSlot()));

    // Disable All
    mHardBPDisableAllAction = new QAction(tr("Disable All"), this);
    mHardBPTable->addAction(mHardBPDisableAllAction);
    connect(mHardBPDisableAllAction, SIGNAL(triggered()), this, SLOT(disableAllHardBPActionSlot()));
}

void BreakpointsView::refreshShortcutsSlot()
{
    mHardBPRemoveAction->setShortcut(ConfigShortcut("ActionDeleteBreakpoint"));
    mHardBPEnableDisableAction->setShortcut(ConfigShortcut("ActionEnableDisableBreakpoint"));

    mSoftBPRemoveAction->setShortcut(ConfigShortcut("ActionDeleteBreakpoint"));
    mSoftBPEnableDisableAction->setShortcut(ConfigShortcut("ActionEnableDisableBreakpoint"));

    mMemBPRemoveAction->setShortcut(ConfigShortcut("ActionDeleteBreakpoint"));
    mMemBPEnableDisableAction->setShortcut(ConfigShortcut("ActionEnableDisableBreakpoint"));

    mEditBreakpointAction->setShortcut(ConfigShortcut("ActionBinaryEdit"));
}

void BreakpointsView::hardwareBPContextMenuSlot(const QPoint & pos)
{
    StdTable* table = mHardBPTable;
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        QMenu wMenu(this);
        duint wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
        BPMAP wBPList;

        // Remove
        wMenu.addAction(mHardBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_hardware, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(wBPList.bp[wI].addr == wVA)
            {
                if(wBPList.bp[wI].active == false)
                {
                    mHardBPEnableDisableAction->setText(tr("Enable"));
                    wMenu.addAction(mHardBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mHardBPEnableDisableAction->setText(tr("Disable"));
                    wMenu.addAction(mHardBPEnableDisableAction);
                }
                else
                {
                    mHardBPEnableDisableAction->setText(tr("Enable"));
                    wMenu.addAction(mHardBPEnableDisableAction);
                }
            }
        }
        if(wBPList.count)
            BridgeFree(wBPList.bp);

        // Conditional
        mCurrentType = bp_hardware;
        wMenu.addAction(mEditBreakpointAction);
        wMenu.addAction(mHardBPResetHitCountAction);

        // Separator
        wMenu.addSeparator();

        // Enable All
        wMenu.addAction(mHardBPEnableAllAction);

        // Disable All
        wMenu.addAction(mHardBPDisableAllAction);

        // Remove All
        wMenu.addAction(mHardBPRemoveAllAction);

        //Copy
        QMenu wCopyMenu(tr("&Copy"), this);
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu.addSeparator();
            wMenu.addMenu(&wCopyMenu);
        }

        wMenu.exec(table->mapToGlobal(pos));
    }
}

void BreakpointsView::removeHardBPActionSlot()
{
    StdTable* table = mHardBPTable;
    duint wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
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
    table->selectNext();
}

void BreakpointsView::enableAllHardBPActionSlot()
{
    DbgCmdExec("bphwe");
}

void BreakpointsView::disableAllHardBPActionSlot()
{
    DbgCmdExec("bphwd");
}

void BreakpointsView::doubleClickHardwareSlot()
{
    StdTable* table = mHardBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    emit showCpu();
}

void BreakpointsView::selectionChangedHardwareSlot()
{
    mCurrentType = bp_hardware;
}

void BreakpointsView::resetHardwareHitCountSlot()
{
    StdTable* table = mHardBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("ResetHardwareBreakpointHitCount " + addrText).toUtf8().constData());
    reloadData();
}

/************************************************************************************
                         Software Context Menu Management
************************************************************************************/
void BreakpointsView::setupSoftBPRightClickContextMenu()
{
    // Remove
    mSoftBPRemoveAction = new QAction(tr("Remove"), this);
    mSoftBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mSoftBPTable->addAction(mSoftBPRemoveAction);
    connect(mSoftBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeSoftBPActionSlot()));

    // Remove All
    mSoftBPRemoveAllAction = new QAction(tr("Remove All"), this);
    connect(mSoftBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllSoftBPActionSlot()));

    // Enable/Disable
    mSoftBPEnableDisableAction = new QAction(tr("Enable"), this);
    mSoftBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mSoftBPTable->addAction(mSoftBPEnableDisableAction);
    connect(mSoftBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableSoftBPActionSlot()));

    // Reset hit count
    mSoftBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mSoftBPTable->addAction(mSoftBPResetHitCountAction);
    connect(mSoftBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetSoftwareHitCountSlot()));

    // Enable All
    mSoftBPEnableAllAction = new QAction(tr("Enable All"), this);
    mSoftBPTable->addAction(mSoftBPEnableAllAction);
    connect(mSoftBPEnableAllAction, SIGNAL(triggered()), this, SLOT(enableAllSoftBPActionSlot()));

    // Disable All
    mSoftBPDisableAllAction = new QAction(tr("Disable All"), this);
    mSoftBPTable->addAction(mSoftBPDisableAllAction);
    connect(mSoftBPDisableAllAction, SIGNAL(triggered()), this, SLOT(disableAllSoftBPActionSlot()));
}

void BreakpointsView::softwareBPContextMenuSlot(const QPoint & pos)
{
    StdTable* table = mSoftBPTable;
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        QMenu wMenu(this);
        duint wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
        BPMAP wBPList;

        // Remove
        wMenu.addAction(mSoftBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_normal, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(wBPList.bp[wI].addr == wVA)
            {
                if(wBPList.bp[wI].active == false)
                {
                    mSoftBPEnableDisableAction->setText(tr("Enable"));
                    wMenu.addAction(mSoftBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mSoftBPEnableDisableAction->setText(tr("Disable"));
                    wMenu.addAction(mSoftBPEnableDisableAction);
                }
                else
                {
                    mSoftBPEnableDisableAction->setText(tr("Enable"));
                    wMenu.addAction(mSoftBPEnableDisableAction);
                }
            }
        }
        if(wBPList.count)
            BridgeFree(wBPList.bp);

        // Conditional
        mCurrentType = bp_normal;
        wMenu.addAction(mEditBreakpointAction);
        wMenu.addAction(mSoftBPResetHitCountAction);

        // Separator
        wMenu.addSeparator();

        // Enable All
        wMenu.addAction(mSoftBPEnableAllAction);

        // Disable All
        wMenu.addAction(mSoftBPDisableAllAction);

        // Remove All
        wMenu.addAction(mSoftBPRemoveAllAction);

        //Copy
        QMenu wCopyMenu(tr("&Copy"), this);
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu.addSeparator();
            wMenu.addMenu(&wCopyMenu);
        }

        wMenu.exec(table->mapToGlobal(pos));
    }
}

void BreakpointsView::removeSoftBPActionSlot()
{
    StdTable* table = mSoftBPTable;
    duint wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
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
    table->selectNext();
}

void BreakpointsView::enableAllSoftBPActionSlot()
{
    DbgCmdExec("bpe");
}

void BreakpointsView::disableAllSoftBPActionSlot()
{
    DbgCmdExec("bpd");
}

void BreakpointsView::doubleClickSoftwareSlot()
{
    StdTable* table = mSoftBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    emit showCpu();
}

void BreakpointsView::selectionChangedSoftwareSlot()
{
    mCurrentType = bp_normal;
}

void BreakpointsView::resetSoftwareHitCountSlot()
{
    StdTable* table = mSoftBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("ResetBreakpointHitCount " + addrText).toUtf8().constData());
    reloadData();
}

/************************************************************************************
                         Memory Context Menu Management
************************************************************************************/
void BreakpointsView::setupMemBPRightClickContextMenu()
{
    // Remove
    mMemBPRemoveAction = new QAction(tr("Remove"), this);
    mMemBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mMemBPTable->addAction(mMemBPRemoveAction);
    connect(mMemBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeMemBPActionSlot()));

    // Remove All
    mMemBPRemoveAllAction = new QAction(tr("Remove All"), this);
    connect(mMemBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllMemBPActionSlot()));

    // Enable/Disable
    mMemBPEnableDisableAction = new QAction(tr("Enable"), this);
    mMemBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mMemBPTable->addAction(mMemBPEnableDisableAction);
    connect(mMemBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableMemBPActionSlot()));

    // Reset hit count
    mMemBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mMemBPTable->addAction(mMemBPResetHitCountAction);
    connect(mMemBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetMemoryHitCountSlot()));

    // Enable All
    mMemBPEnableAllAction = new QAction(tr("Enable All"), this);
    mMemBPTable->addAction(mMemBPEnableAllAction);
    connect(mMemBPEnableAllAction, SIGNAL(triggered()), this, SLOT(enableAllMemBPActionSlot()));

    // Disable All
    mMemBPDisableAllAction = new QAction(tr("Disable All"), this);
    mMemBPTable->addAction(mMemBPDisableAllAction);
    connect(mMemBPDisableAllAction, SIGNAL(triggered()), this, SLOT(disableAllMemBPActionSlot()));
}

void BreakpointsView::memoryBPContextMenuSlot(const QPoint & pos)
{
    StdTable* table = mMemBPTable;
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        QMenu wMenu(this);
        duint wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
        BPMAP wBPList;

        // Remove
        wMenu.addAction(mMemBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_memory, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(wBPList.bp[wI].addr == wVA)
            {
                if(wBPList.bp[wI].active == false)
                {
                    mMemBPEnableDisableAction->setText(tr("Enable"));
                    wMenu.addAction(mMemBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mMemBPEnableDisableAction->setText(tr("Disable"));
                    wMenu.addAction(mMemBPEnableDisableAction);
                }
                else
                {
                    mMemBPEnableDisableAction->setText(tr("Enable"));
                    wMenu.addAction(mMemBPEnableDisableAction);
                }
            }
        }
        if(wBPList.count)
            BridgeFree(wBPList.bp);

        // Conditional
        mCurrentType = bp_memory;
        wMenu.addAction(mEditBreakpointAction);
        wMenu.addAction(mMemBPResetHitCountAction);

        // Separator
        wMenu.addSeparator();

        // Enable All
        wMenu.addAction(mMemBPEnableAllAction);

        // Disable All
        wMenu.addAction(mMemBPDisableAllAction);

        // Remove All
        wMenu.addAction(mMemBPRemoveAllAction);

        //Copy
        QMenu wCopyMenu(tr("&Copy"), this);
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu.addSeparator();
            wMenu.addMenu(&wCopyMenu);
        }

        wMenu.exec(table->mapToGlobal(pos));
    }
}

void BreakpointsView::removeMemBPActionSlot()
{
    StdTable* table = mMemBPTable;
    duint wVA = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
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
    table->selectNext();
}

void BreakpointsView::enableAllMemBPActionSlot()
{
    DbgCmdExec("bpme");
}

void BreakpointsView::disableAllMemBPActionSlot()
{
    DbgCmdExec("bpmd");
}

void BreakpointsView::doubleClickMemorySlot()
{
    StdTable* table = mMemBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    emit showCpu();
}

void BreakpointsView::selectionChangedMemorySlot()
{
    mCurrentType = bp_memory;
}

void BreakpointsView::resetMemoryHitCountSlot()
{
    StdTable* table = mMemBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("ResetMemoryBreakpointHitCount " + addrText).toUtf8().constData());
    reloadData();
}


/************************************************************************************
                         DLL Context Menu Management
************************************************************************************/
void BreakpointsView::setupDLLBPRightClickContextMenu()
{
    // Remove
    mDLLBPRemoveAction = new QAction(tr("Remove"), this);
    mDLLBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mDLLBPTable->addAction(mDLLBPRemoveAction);
    connect(mDLLBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeDLLBPActionSlot()));

    // Enable/Disable
    mDLLBPEnableDisableAction = new QAction(tr("Enable"), this);
    mDLLBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mDLLBPTable->addAction(mDLLBPEnableDisableAction);
    connect(mDLLBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableDLLBPActionSlot()));

    // Reset hit count
    mDLLBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mDLLBPTable->addAction(mDLLBPResetHitCountAction);
    connect(mDLLBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetDLLHitCountSlot()));
}

void BreakpointsView::DLLBPContextMenuSlot(const QPoint & pos)
{
    StdTable* table = mDLLBPTable;
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        QMenu wMenu(this);
        QString module = table->getCellContent(table->getInitialSelection(), 1);
        BPMAP wBPList;

        // Remove
        wMenu.addAction(mDLLBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_dll, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(QString::fromUtf8(wBPList.bp[wI].mod) == module)
            {
                if(wBPList.bp[wI].active == false)
                {
                    mDLLBPEnableDisableAction->setText(tr("Enable"));
                    wMenu.addAction(mDLLBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mDLLBPEnableDisableAction->setText(tr("Disable"));
                    wMenu.addAction(mDLLBPEnableDisableAction);
                }
                else
                {
                    mDLLBPEnableDisableAction->setText(tr("Enable"));
                    wMenu.addAction(mDLLBPEnableDisableAction);
                }
            }
        }
        if(wBPList.count)
            BridgeFree(wBPList.bp);

        // Conditional
        mCurrentType = bp_dll;
        wMenu.addAction(mEditBreakpointAction);
        wMenu.addAction(mDLLBPResetHitCountAction);

        // Separator
        wMenu.addSeparator();

        //Copy
        QMenu wCopyMenu(tr("&Copy"), this);
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu.addSeparator();
            wMenu.addMenu(&wCopyMenu);
        }

        wMenu.exec(table->mapToGlobal(pos));
    }
}

void BreakpointsView::removeDLLBPActionSlot()
{
    StdTable* table = mDLLBPTable;
    Breakpoints::removeBP(table->getCellContent(table->getInitialSelection(), 1));
}

void BreakpointsView::enableDisableDLLBPActionSlot()
{
    StdTable* table = mDLLBPTable;
    Breakpoints::toggleBPByDisabling(table->getCellContent(table->getInitialSelection(), 1));
    table->selectNext();
}

void BreakpointsView::selectionChangedDLLSlot()
{
    mCurrentType = bp_dll;
}

void BreakpointsView::resetDLLHitCountSlot()
{
    StdTable* table = mDLLBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("ResetLibrarianBreakpointHitCount " + addrText).toUtf8().constData());
    reloadData();
}

/************************************************************************************
           Conditional Breakpoint Context Menu Management (Sub-menu only)
************************************************************************************/
void BreakpointsView::editBreakpointSlot()
{
    StdTable* table;
    switch(mCurrentType)
    {
    case bp_normal:
        table = mSoftBPTable;
        break;
    case bp_hardware:
        table = mHardBPTable;
        break;
    case bp_memory:
        table = mMemBPTable;
        break;
    case bp_dll:
        table = mDLLBPTable;
        Breakpoints::editBP(mCurrentType, table->getCellContent(table->getInitialSelection(), 1), this);
        return;
    default:
        return;
    }
    Breakpoints::editBP(mCurrentType, table->getCellContent(table->getInitialSelection(), 0), this);
}

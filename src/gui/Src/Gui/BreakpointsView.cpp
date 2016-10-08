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
    mSoftBPTable->setWindowTitle("SoftwareBreakpoints");
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
    mHardBPTable->setWindowTitle("HardwareBreakpoints");
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
    mMemBPTable->setWindowTitle("MemoryBreakpoints");
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
    mDLLBPTable->setWindowTitle("DllBreakpoints");
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

    // Exception
    mExceptionBPTable = new StdTable(this);
    mExceptionBPTable->setWindowTitle("ExceptionBreakpoints");
    mExceptionBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mExceptionBPTable->addColumnAt(8 + wCharWidth * 2 * sizeof(duint), tr("Exception Code"), false);
    mExceptionBPTable->addColumnAt(8 + wCharWidth * 32, tr("Name"), false);
    mExceptionBPTable->addColumnAt(8 + wCharWidth * 8, tr("State"), false);
    mExceptionBPTable->addColumnAt(8 + wCharWidth * 10, tr("Hit count"), false);
    mExceptionBPTable->addColumnAt(8 + wCharWidth * 32, tr("Log text"), false);
    mExceptionBPTable->addColumnAt(8 + wCharWidth * 32, tr("Condition"), false);
    mExceptionBPTable->addColumnAt(8 + wCharWidth * 2, tr("Chance"), false);
    mExceptionBPTable->addColumnAt(8 + wCharWidth * 16, tr("Command on hit"), false);
    mExceptionBPTable->loadColumnFromConfig("ExceptionBreakpoint");

    // Splitter
    mSplitter = new LabeledSplitter(this);
    mSplitter->addWidget(mSoftBPTable, tr("Software breakpoint"));
    mSplitter->addWidget(mHardBPTable, tr("Hardware breakpoint"));
    mSplitter->addWidget(mMemBPTable, tr("Memory breakpoint"));
    mSplitter->addWidget(mExceptionBPTable, tr("Exception breakpoint"));
    mSplitter->addWidget(mDLLBPTable, tr("DLL breakpoint"));
    mSplitter->collapseLowerTabs();

    // Layout
    mVertLayout = new QVBoxLayout;
    mVertLayout->setSpacing(0);
    mVertLayout->setContentsMargins(0, 0, 0, 0);
    mVertLayout->addWidget(mSplitter);
    this->setLayout(mVertLayout);
    mSplitter->loadFromConfig("BreakpointsViewSplitter");

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
    connect(mExceptionBPTable, SIGNAL(contextMenuSignal(const QPoint &)), this, SLOT(ExceptionBPContextMenuSlot(const QPoint &)));
    connect(mExceptionBPTable, SIGNAL(selectionChangedSignal(int)), this, SLOT(selectionChangedExceptionSlot()));

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
        QString addr_text = ToPtrString(wBPList.bp[wI].addr);
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
            mHardBPTable->setCellContent(wI, 3, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mHardBPTable->setCellContent(wI, 3, tr("Enabled"));
        else
            mHardBPTable->setCellContent(wI, 3, tr("Disabled"));

        mHardBPTable->setCellContent(wI, 4, QString("%1").arg(wBPList.bp[wI].hitCount));
        mHardBPTable->setCellContent(wI, 5, QString::fromUtf8(wBPList.bp[wI].logText));
        mHardBPTable->setCellContent(wI, 6, QString::fromUtf8(wBPList.bp[wI].breakCondition));
        mHardBPTable->setCellContent(wI, 7, wBPList.bp[wI].fastResume ? "X" : "");
        mHardBPTable->setCellContent(wI, 8, QString::fromUtf8(wBPList.bp[wI].commandText));

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
        QString addr_text = ToPtrString(wBPList.bp[wI].addr);
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
            mSoftBPTable->setCellContent(wI, 3, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mSoftBPTable->setCellContent(wI, 3, tr("Enabled"));
        else
            mSoftBPTable->setCellContent(wI, 3, tr("Disabled"));

        mSoftBPTable->setCellContent(wI, 4, QString("%1").arg(wBPList.bp[wI].hitCount));
        mSoftBPTable->setCellContent(wI, 5, QString::fromUtf8(wBPList.bp[wI].logText));
        mSoftBPTable->setCellContent(wI, 6, QString::fromUtf8(wBPList.bp[wI].breakCondition));
        mSoftBPTable->setCellContent(wI, 7, wBPList.bp[wI].fastResume ? "X" : "");
        mSoftBPTable->setCellContent(wI, 8, QString::fromUtf8(wBPList.bp[wI].commandText));

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
        QString addr_text = ToPtrString(wBPList.bp[wI].addr);
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
            mMemBPTable->setCellContent(wI, 3, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mMemBPTable->setCellContent(wI, 3, tr("Enabled"));
        else
            mMemBPTable->setCellContent(wI, 3, tr("Disabled"));

        mMemBPTable->setCellContent(wI, 4, QString("%1").arg(wBPList.bp[wI].hitCount));
        mMemBPTable->setCellContent(wI, 5, QString::fromUtf8(wBPList.bp[wI].logText));
        mMemBPTable->setCellContent(wI, 6, QString::fromUtf8(wBPList.bp[wI].breakCondition));
        mMemBPTable->setCellContent(wI, 7, wBPList.bp[wI].fastResume ? "X" : "");
        mMemBPTable->setCellContent(wI, 8, QString::fromUtf8(wBPList.bp[wI].commandText));

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
        mDLLBPTable->setCellContent(wI, 0, QString(wBPList.bp[wI].name));
        mDLLBPTable->setCellContent(wI, 1, QString(wBPList.bp[wI].mod));

        if(wBPList.bp[wI].active == false)
            mDLLBPTable->setCellContent(wI, 2, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mDLLBPTable->setCellContent(wI, 2, tr("Enabled"));
        else
            mDLLBPTable->setCellContent(wI, 2, tr("Disabled"));

        mDLLBPTable->setCellContent(wI, 3, QString("%1").arg(wBPList.bp[wI].hitCount));
        mDLLBPTable->setCellContent(wI, 4, QString::fromUtf8(wBPList.bp[wI].logText));
        mDLLBPTable->setCellContent(wI, 5, QString::fromUtf8(wBPList.bp[wI].breakCondition));
        mDLLBPTable->setCellContent(wI, 6, wBPList.bp[wI].fastResume ? "X" : "");
        mDLLBPTable->setCellContent(wI, 7, QString::fromUtf8(wBPList.bp[wI].commandText));
    }
    mDLLBPTable->reloadData();

    // Exception
    DbgGetBpList(bp_exception, &wBPList);
    mExceptionBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        mExceptionBPTable->setCellContent(wI, 0, ToPtrString(wBPList.bp[wI].addr));
        mExceptionBPTable->setCellContent(wI, 1, QString::fromUtf8(wBPList.bp[wI].name));
        if(wBPList.bp[wI].active == false)
            mExceptionBPTable->setCellContent(wI, 2, tr("Inactive"));
        else if(wBPList.bp[wI].enabled == true)
            mExceptionBPTable->setCellContent(wI, 2, tr("Enabled"));
        else
            mExceptionBPTable->setCellContent(wI, 2, tr("Disabled"));
        mExceptionBPTable->setCellContent(wI, 3, QString("%1").arg(wBPList.bp[wI].hitCount));
        mExceptionBPTable->setCellContent(wI, 4, QString::fromUtf8(wBPList.bp[wI].logText));
        mExceptionBPTable->setCellContent(wI, 5, QString::fromUtf8(wBPList.bp[wI].breakCondition));
        if(wBPList.bp[wI].slot == 1)
            mExceptionBPTable->setCellContent(wI, 6, tr("First-chance"));
        else if(wBPList.bp[wI].slot == 2)
            mExceptionBPTable->setCellContent(wI, 6, tr("Second-chance"));
        else if(wBPList.bp[wI].slot == 3)
            mExceptionBPTable->setCellContent(wI, 6, tr("All")); // both first-chance and second-chance
        mExceptionBPTable->setCellContent(wI, 7, QString::fromUtf8(wBPList.bp[wI].commandText));
    }

    if(wBPList.count)
        BridgeFree(wBPList.bp);
    mExceptionBPTable->reloadData();
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
    setupExceptionBPRightClickContextMenu();
}

/************************************************************************************
                         Hardware Context Menu Management
************************************************************************************/
void BreakpointsView::setupHardBPRightClickContextMenu()
{
    // Remove
    mHardBPRemoveAction = new QAction(tr("&Remove"), this);
    mHardBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mHardBPTable->addAction(mHardBPRemoveAction);
    connect(mHardBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeHardBPActionSlot()));

    // Remove All
    mHardBPRemoveAllAction = new QAction(tr("Remove All"), this);
    connect(mHardBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllHardBPActionSlot()));

    // Enable/Disable
    mHardBPEnableDisableAction = new QAction(DIcon("enable.png"), tr("E&nable"), this);
    mHardBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mHardBPTable->addAction(mHardBPEnableDisableAction);
    connect(mHardBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableHardBPActionSlot()));

    // Reset hit count
    mHardBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mHardBPTable->addAction(mHardBPResetHitCountAction);
    connect(mHardBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetHardwareHitCountSlot()));

    // Enable All
    mHardBPEnableAllAction = new QAction(DIcon("enable.png"), tr("Enable All"), this);
    mHardBPTable->addAction(mHardBPEnableAllAction);
    connect(mHardBPEnableAllAction, SIGNAL(triggered()), this, SLOT(enableAllHardBPActionSlot()));

    // Disable All
    mHardBPDisableAllAction = new QAction(DIcon("disable.png"), tr("Disable All"), this);
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
                    mHardBPEnableDisableAction->setText(tr("E&nable"));
                    mHardBPEnableDisableAction->setIcon(DIcon("enable.png"));
                    wMenu.addAction(mHardBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mHardBPEnableDisableAction->setText(tr("&Disable"));
                    mHardBPEnableDisableAction->setIcon(DIcon("disable.png"));
                    wMenu.addAction(mHardBPEnableDisableAction);
                }
                else
                {
                    mHardBPEnableDisableAction->setText(tr("E&nable"));
                    mHardBPEnableDisableAction->setIcon(DIcon("enable.png"));
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
        wCopyMenu.setIcon(DIcon("copy.png"));
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
    mSoftBPRemoveAction = new QAction(tr("&Remove"), this);
    mSoftBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mSoftBPTable->addAction(mSoftBPRemoveAction);
    connect(mSoftBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeSoftBPActionSlot()));

    // Remove All
    mSoftBPRemoveAllAction = new QAction(tr("Remove All"), this);
    connect(mSoftBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllSoftBPActionSlot()));

    // Enable/Disable
    mSoftBPEnableDisableAction = new QAction(DIcon("enable.png"), tr("E&nable"), this);
    mSoftBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mSoftBPTable->addAction(mSoftBPEnableDisableAction);
    connect(mSoftBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableSoftBPActionSlot()));

    // Reset hit count
    mSoftBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mSoftBPTable->addAction(mSoftBPResetHitCountAction);
    connect(mSoftBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetSoftwareHitCountSlot()));

    // Enable All
    mSoftBPEnableAllAction = new QAction(DIcon("enable.png"), tr("Enable All"), this);
    mSoftBPTable->addAction(mSoftBPEnableAllAction);
    connect(mSoftBPEnableAllAction, SIGNAL(triggered()), this, SLOT(enableAllSoftBPActionSlot()));

    // Disable All
    mSoftBPDisableAllAction = new QAction(DIcon("disable.png"), tr("Disable All"), this);
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
                    mSoftBPEnableDisableAction->setText(tr("E&nable"));
                    mSoftBPEnableDisableAction->setIcon(DIcon("enable.png"));
                    wMenu.addAction(mSoftBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mSoftBPEnableDisableAction->setText(tr("&Disable"));
                    mSoftBPEnableDisableAction->setIcon(DIcon("disable.png"));
                    wMenu.addAction(mSoftBPEnableDisableAction);
                }
                else
                {
                    mSoftBPEnableDisableAction->setText(tr("E&nable"));
                    mSoftBPEnableDisableAction->setIcon(DIcon("enable.png"));
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
        wCopyMenu.setIcon(DIcon("copy.png"));
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
    mMemBPRemoveAction = new QAction(tr("&Remove"), this);
    mMemBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mMemBPTable->addAction(mMemBPRemoveAction);
    connect(mMemBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeMemBPActionSlot()));

    // Remove All
    mMemBPRemoveAllAction = new QAction(tr("Remove All"), this);
    connect(mMemBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllMemBPActionSlot()));

    // Enable/Disable
    mMemBPEnableDisableAction = new QAction(DIcon("enable.png"), tr("E&nable"), this);
    mMemBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mMemBPTable->addAction(mMemBPEnableDisableAction);
    connect(mMemBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableMemBPActionSlot()));

    // Reset hit count
    mMemBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mMemBPTable->addAction(mMemBPResetHitCountAction);
    connect(mMemBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetMemoryHitCountSlot()));

    // Enable All
    mMemBPEnableAllAction = new QAction(DIcon("enable.png"), tr("Enable All"), this);
    mMemBPTable->addAction(mMemBPEnableAllAction);
    connect(mMemBPEnableAllAction, SIGNAL(triggered()), this, SLOT(enableAllMemBPActionSlot()));

    // Disable All
    mMemBPDisableAllAction = new QAction(DIcon("disable.png"), tr("Disable All"), this);
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
                    mMemBPEnableDisableAction->setText(tr("E&nable"));
                    mMemBPEnableDisableAction->setIcon(DIcon("enable.png"));
                    wMenu.addAction(mMemBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mMemBPEnableDisableAction->setText(tr("&Disable"));
                    mMemBPEnableDisableAction->setIcon(DIcon("disable.png"));
                    wMenu.addAction(mMemBPEnableDisableAction);
                }
                else
                {
                    mMemBPEnableDisableAction->setText(tr("E&nable"));
                    mMemBPEnableDisableAction->setIcon(DIcon("enable.png"));
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
        wCopyMenu.setIcon(DIcon("copy.png"));
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
    // Add
    mDLLBPAddAction = new QAction(tr("&Add"), this);
    connect(mDLLBPAddAction, SIGNAL(triggered()), this, SLOT(addDLLBPActionSlot()));

    // Remove
    mDLLBPRemoveAction = new QAction(tr("&Remove"), this);
    mDLLBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mDLLBPTable->addAction(mDLLBPRemoveAction);
    connect(mDLLBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeDLLBPActionSlot()));

    // Remove All
    mDLLBPRemoveAllAction = new QAction(tr("Remove All"), this);
    connect(mDLLBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllDLLBPActionSlot()));

    // Enable/Disable
    mDLLBPEnableDisableAction = new QAction(tr("E&nable"), this);
    mDLLBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mDLLBPTable->addAction(mDLLBPEnableDisableAction);
    connect(mDLLBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableDLLBPActionSlot()));

    // Reset hit count
    mDLLBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mDLLBPTable->addAction(mDLLBPResetHitCountAction);
    connect(mDLLBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetDLLHitCountSlot()));

    // Enable All
    mDLLBPEnableAllAction = new QAction(DIcon("enable.png"), tr("Enable All"), this);
    mDLLBPTable->addAction(mDLLBPEnableAllAction);
    connect(mDLLBPEnableAllAction, SIGNAL(triggered()), this, SLOT(enableAllDLLBPActionSlot()));

    // Disable All
    mDLLBPDisableAllAction = new QAction(DIcon("disable.png"), ("Disable All"), this);
    mDLLBPTable->addAction(mDLLBPDisableAllAction);
    connect(mDLLBPDisableAllAction, SIGNAL(triggered()), this, SLOT(disableAllDLLBPActionSlot()));
}

void BreakpointsView::DLLBPContextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;
    StdTable* table = mDLLBPTable;
    QMenu wMenu(this);
    wMenu.addAction(mDLLBPAddAction);
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        QString wName = table->getCellContent(table->getInitialSelection(), 1);
        BPMAP wBPList;

        // Remove
        wMenu.addAction(mDLLBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_dll, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(QString(wBPList.bp[wI].mod) == wName)
            {
                if(wBPList.bp[wI].active == false)
                {
                    mDLLBPEnableDisableAction->setText(tr("E&nable"));
                    mDLLBPEnableDisableAction->setIcon(DIcon("enable.png"));
                    wMenu.addAction(mDLLBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mDLLBPEnableDisableAction->setText(tr("&Disable"));
                    mDLLBPEnableDisableAction->setIcon(DIcon("disable.png"));
                    wMenu.addAction(mDLLBPEnableDisableAction);
                }
                else
                {
                    mDLLBPEnableDisableAction->setText(tr("E&nable"));
                    mDLLBPEnableDisableAction->setIcon(DIcon("enable.png"));
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

        // Enable All
        wMenu.addAction(mDLLBPEnableAllAction);

        // Disable All
        wMenu.addAction(mDLLBPDisableAllAction);

        // Remove All
        wMenu.addAction(mDLLBPRemoveAllAction);

        //Copy
        QMenu wCopyMenu(tr("&Copy"), this);
        wCopyMenu.setIcon(DIcon("copy.png"));
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu.addSeparator();
            wMenu.addMenu(&wCopyMenu);
        }

    }
    wMenu.exec(table->mapToGlobal(pos));
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
    QString addrText = table->getCellContent(table->getInitialSelection(), 1);
    DbgCmdExecDirect(QString("ResetLibrarianBreakpointHitCount \"%1\"").arg(addrText).toUtf8().constData());
    reloadData();
}

void BreakpointsView::addDLLBPActionSlot()
{
    QString fileName;
    if(SimpleInputBox(this, tr("Enter the module name"), "", fileName, tr("Example: mydll.dll"), &DIcon("breakpoint.png")) && !fileName.isEmpty())
    {
        DbgCmdExec((QString("bpdll ") + fileName).toUtf8().constData());
    }
}

void BreakpointsView::enableAllDLLBPActionSlot()
{
    DbgCmdExec("LibrarianEnableBreakPoint");
}

void BreakpointsView::disableAllDLLBPActionSlot()
{
    DbgCmdExec("LibrarianDisableBreakPoint");
}

void BreakpointsView::removeAllDLLBPActionSlot()
{
    DbgCmdExec("LibrarianRemoveBreakPoint");
}


/************************************************************************************
                         Exception Context Menu Management
************************************************************************************/
void BreakpointsView::setupExceptionBPRightClickContextMenu()
{
    // Add
    mExceptionBPAddAction = new QAction(tr("&Add"), this);
    connect(mExceptionBPAddAction, SIGNAL(triggered()), this, SLOT(addExceptionBPActionSlot()));

    // Remove
    mExceptionBPRemoveAction = new QAction(tr("&Remove"), this);
    mExceptionBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mExceptionBPTable->addAction(mExceptionBPRemoveAction);
    connect(mExceptionBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeExceptionBPActionSlot()));

    // Remove All
    mExceptionBPRemoveAllAction = new QAction(tr("Remove All"), this);
    connect(mExceptionBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllExceptionBPActionSlot()));

    // Enable/Disable
    mExceptionBPEnableDisableAction = new QAction(tr("E&nable"), this);
    mExceptionBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mExceptionBPTable->addAction(mExceptionBPEnableDisableAction);
    connect(mExceptionBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableExceptionBPActionSlot()));

    // Reset hit count
    mExceptionBPResetHitCountAction = new QAction(tr("Reset hit count"), this);
    mExceptionBPTable->addAction(mExceptionBPResetHitCountAction);
    connect(mExceptionBPResetHitCountAction, SIGNAL(triggered()), this, SLOT(resetExceptionHitCountSlot()));

    // Enable All
    mExceptionBPEnableAllAction = new QAction(tr("Enable All"), this);
    mExceptionBPTable->addAction(mExceptionBPEnableAllAction);
    connect(mExceptionBPEnableAllAction, SIGNAL(triggered()), this, SLOT(enableAllExceptionBPActionSlot()));

    // Disable All
    mExceptionBPDisableAllAction = new QAction(tr("Disable All"), this);
    mExceptionBPTable->addAction(mExceptionBPDisableAllAction);
    connect(mExceptionBPDisableAllAction, SIGNAL(triggered()), this, SLOT(disableAllExceptionBPActionSlot()));
}

void BreakpointsView::ExceptionBPContextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;
    StdTable* table = mExceptionBPTable;
    QMenu wMenu(this);
    wMenu.addAction(mExceptionBPAddAction);
    if(table->getRowCount() != 0)
    {
        int wI = 0;
        duint wExceptionCode = table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16);
        BPMAP wBPList;

        // Remove
        wMenu.addAction(mExceptionBPRemoveAction);

        // Enable/Disable
        DbgGetBpList(bp_exception, &wBPList);

        for(wI = 0; wI < wBPList.count; wI++)
        {
            if(wBPList.bp[wI].addr == wExceptionCode)
            {
                if(wBPList.bp[wI].active == false)
                {
                    mExceptionBPEnableDisableAction->setText(tr("E&nable"));
                    wMenu.addAction(mExceptionBPEnableDisableAction);
                }
                else if(wBPList.bp[wI].enabled == true)
                {
                    mExceptionBPEnableDisableAction->setText(tr("&Disable"));
                    wMenu.addAction(mExceptionBPEnableDisableAction);
                }
                else
                {
                    mExceptionBPEnableDisableAction->setText(tr("E&nable"));
                    wMenu.addAction(mExceptionBPEnableDisableAction);
                }
            }
        }
        if(wBPList.count)
            BridgeFree(wBPList.bp);

        // Conditional
        mCurrentType = bp_exception;
        wMenu.addAction(mEditBreakpointAction);
        wMenu.addAction(mExceptionBPResetHitCountAction);

        // Separator
        wMenu.addSeparator();

        // Enable All
        wMenu.addAction(mExceptionBPEnableAllAction);

        // Disable All
        wMenu.addAction(mExceptionBPDisableAllAction);

        // Remove All
        wMenu.addAction(mExceptionBPRemoveAllAction);

        //Copy
        QMenu wCopyMenu(tr("&Copy"), this);
        table->setupCopyMenu(&wCopyMenu);
        if(wCopyMenu.actions().length())
        {
            wMenu.addSeparator();
            wMenu.addMenu(&wCopyMenu);
        }

    }
    wMenu.exec(table->mapToGlobal(pos));
}

void BreakpointsView::removeExceptionBPActionSlot()
{
    StdTable* table = mExceptionBPTable;
    Breakpoints::removeBP(bp_exception, table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16));
}

void BreakpointsView::enableDisableExceptionBPActionSlot()
{
    StdTable* table = mExceptionBPTable;
    Breakpoints::toggleBPByDisabling(bp_exception, table->getCellContent(table->getInitialSelection(), 0).toULongLong(0, 16));
    table->selectNext();
}

void BreakpointsView::selectionChangedExceptionSlot()
{
    mCurrentType = bp_exception;
}

void BreakpointsView::resetExceptionHitCountSlot()
{
    StdTable* table = mExceptionBPTable;
    QString addrText = table->getCellContent(table->getInitialSelection(), 0);
    DbgCmdExecDirect(QString("ResetExceptionBreakpointHitCount \"%1\"").arg(addrText).toUtf8().constData());
    reloadData();
}

void BreakpointsView::addExceptionBPActionSlot()
{
    QString fileName;
    if(SimpleInputBox(this, tr("Enter the exception code"), "", fileName, tr("Example: EXCEPTION_ACCESS_VIOLATION"), &DIcon("breakpoint.png")) && !fileName.isEmpty())
    {
        DbgCmdExec((QString("SetExceptionBPX ") + fileName).toUtf8().constData());
    }
}

void BreakpointsView::enableAllExceptionBPActionSlot()
{
    DbgCmdExec("EnableExceptionBPX");
}

void BreakpointsView::disableAllExceptionBPActionSlot()
{
    DbgCmdExec("DisableExceptionBPX");
}

void BreakpointsView::removeAllExceptionBPActionSlot()
{
    DbgCmdExec("DeleteExceptionBPX");
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
    case bp_exception:
        table = mExceptionBPTable;
        break;
    default:
        return;
    }
    Breakpoints::editBP(mCurrentType, table->getCellContent(table->getInitialSelection(), 0), this);
}

#include "BreakpointsView.h"

BreakpointsView::BreakpointsView(QWidget *parent) : QWidget(parent)
{
    // Hardware
    mHardBPTable = new StdTable(this);
    int wCharWidth = QFontMetrics(mHardBPTable->font()).width(QChar(' '));
    mHardBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mHardBPTable->addColumnAt(8+wCharWidth*2*sizeof(uint_t), "Hardware", false);
    mHardBPTable->addColumnAt(8+wCharWidth*32, "Name", false);
    mHardBPTable->addColumnAt(8+wCharWidth*32, "Module/Label", false);
    mHardBPTable->addColumnAt(8+wCharWidth*8, "State", false);
    mHardBPTable->addColumnAt(wCharWidth*10, "Comment", false);

    // Software
    mSoftBPTable = new StdTable(this);
    mSoftBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mSoftBPTable->addColumnAt(8+wCharWidth*2*sizeof(uint_t), "Software", false);
    mSoftBPTable->addColumnAt(8+wCharWidth*32, "Name", false);
    mSoftBPTable->addColumnAt(8+wCharWidth*32, "Module/Label", false);
    mSoftBPTable->addColumnAt(8+wCharWidth*8, "State", false);
    mSoftBPTable->addColumnAt(wCharWidth*10, "Comment", false);

    // Memory
    mMemBPTable = new StdTable(this);
    mMemBPTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mMemBPTable->addColumnAt(8+wCharWidth*2*sizeof(uint_t), "Memory", false);
    mMemBPTable->addColumnAt(8+wCharWidth*32, "Name", false);
    mMemBPTable->addColumnAt(8+wCharWidth*32, "Module/Label", false);
    mMemBPTable->addColumnAt(8+wCharWidth*8, "State", false);
    mMemBPTable->addColumnAt(wCharWidth*10, "Comment", false);

    // Splitter
    mSplitter = new QSplitter(this);
    mSplitter->setOrientation(Qt::Vertical);
    mSplitter->addWidget(mHardBPTable);
    mSplitter->addWidget(mSoftBPTable);
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

    // Signals/Slots
    connect(Bridge::getBridge(), SIGNAL(updateBreakpoints()), this, SLOT(reloadData()));
    connect(mHardBPTable, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(hardwareBPContextMenuSlot(const QPoint &)));
    connect(mSoftBPTable, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(softwareBPContextMenuSlot(const QPoint &)));
    connect(mMemBPTable, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(memoryBPContextMenuSlot(const QPoint &)));
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
        QString addr_text=QString("%1").arg(wBPList.bp[wI].addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        mSoftBPTable->setCellContent(wI, 0, addr_text);
        mHardBPTable->setCellContent(wI, 1, QString(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE]="";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text="<"+QString(wBPList.bp[wI].mod)+"."+QString(label)+">";
        else
            label_text=QString(wBPList.bp[wI].mod);
        mSoftBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mHardBPTable->setCellContent(wI, 3, "Inactive");
        else if(wBPList.bp[wI].enabled == true)
            mHardBPTable->setCellContent(wI, 3, "Enabled");
        else
            mHardBPTable->setCellContent(wI, 3, "Disabled");

        char comment[MAX_COMMENT_SIZE]="";
        if(DbgGetCommentAt(wBPList.bp[wI].addr, comment))
            mSoftBPTable->setCellContent(wI, 4, comment);
    }
    mHardBPTable->reloadData();
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    // Software
    DbgGetBpList(bp_normal, &wBPList);
    mSoftBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        QString addr_text=QString("%1").arg(wBPList.bp[wI].addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        mSoftBPTable->setCellContent(wI, 0, addr_text);
        mSoftBPTable->setCellContent(wI, 1, QString(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE]="";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text="<"+QString(wBPList.bp[wI].mod)+"."+QString(label)+">";
        else
            label_text=QString(wBPList.bp[wI].mod);
        mSoftBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mSoftBPTable->setCellContent(wI, 3, "Inactive");
        else if(wBPList.bp[wI].enabled == true)
            mSoftBPTable->setCellContent(wI, 3, "Enabled");
        else
            mSoftBPTable->setCellContent(wI, 3, "Disabled");

        char comment[MAX_COMMENT_SIZE]="";
        if(DbgGetCommentAt(wBPList.bp[wI].addr, comment))
            mSoftBPTable->setCellContent(wI, 4, comment);
    }
    mSoftBPTable->reloadData();
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    // Memory
    DbgGetBpList(bp_memory, &wBPList);
    mMemBPTable->setRowCount(wBPList.count);
    for(wI = 0; wI < wBPList.count; wI++)
    {
        QString addr_text=QString("%1").arg(wBPList.bp[wI].addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        mSoftBPTable->setCellContent(wI, 0, addr_text);
        mMemBPTable->setCellContent(wI, 1, QString(wBPList.bp[wI].name));

        QString label_text;
        char label[MAX_LABEL_SIZE]="";
        if(DbgGetLabelAt(wBPList.bp[wI].addr, SEG_DEFAULT, label))
            label_text="<"+QString(wBPList.bp[wI].mod)+"."+QString(label)+">";
        else
            label_text=QString(wBPList.bp[wI].mod);
        mSoftBPTable->setCellContent(wI, 2, label_text);

        if(wBPList.bp[wI].active == false)
            mMemBPTable->setCellContent(wI, 3, "Inactive");
        else if(wBPList.bp[wI].enabled == true)
            mMemBPTable->setCellContent(wI, 3, "Enabled");
        else
            mMemBPTable->setCellContent(wI, 3, "Disabled");

        char comment[MAX_COMMENT_SIZE]="";
        if(DbgGetCommentAt(wBPList.bp[wI].addr, comment))
            mSoftBPTable->setCellContent(wI, 4, comment);
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
    mHardBPRemoveAction->setShortcut(QKeySequence(Qt::Key_Delete));
    mHardBPTable->addAction(mHardBPRemoveAction);
    connect(mHardBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeHardBPActionSlot()));

    // Remove All
    mHardBPRemoveAllAction = new QAction("Remove All", this);
    connect(mHardBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllHardBPActionSlot()));

    // Enable/Disable
    mHardBPEnableDisableAction = new QAction("Enable", this);
    mHardBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mHardBPEnableDisableAction->setShortcut(QKeySequence(Qt::Key_Space));
    mHardBPTable->addAction(mHardBPEnableDisableAction);
    connect(mHardBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableHardBPActionSlot()));
}

void BreakpointsView::hardwareBPContextMenuSlot(const QPoint & pos)
{
    if(mHardBPTable->getRowCount() != 0)
    {
        int wI = 0;
        QMenu* wMenu = new QMenu(this);
        uint_t wVA = mHardBPTable->getCellContent(mHardBPTable->getInitialSelection(), 0).toULongLong(0, 16);
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

        // Separator
        wMenu->addSeparator();

        // Remove All
        wMenu->addAction(mHardBPRemoveAllAction);

        QAction* wAction = wMenu->exec(mHardBPTable->mapToGlobal(pos));
    }
}

void BreakpointsView::removeHardBPActionSlot()
{
    qDebug() << "mHardBPTable->getInitialSelection()" << mHardBPTable->getInitialSelection();
    uint_t wVA = mHardBPTable->getCellContent(mHardBPTable->getInitialSelection(), 0).toULongLong(0, 16);
    Breakpoints::removeBP(bp_hardware, wVA);
}

void BreakpointsView::removeAllHardBPActionSlot()
{

}

void BreakpointsView::enableDisableHardBPActionSlot()
{
    Breakpoints::toogleBPByDisabling(bp_hardware, mHardBPTable->getCellContent(mHardBPTable->getInitialSelection(), 0).toULongLong(0, 16));
}


/************************************************************************************
                         Software Context Menu Management
************************************************************************************/
void BreakpointsView::setupSoftBPRightClickContextMenu()
{
    // Remove
    mSoftBPRemoveAction = new QAction("Remove", this);
    mSoftBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mSoftBPRemoveAction->setShortcut(QKeySequence(Qt::Key_Delete));
    mSoftBPTable->addAction(mSoftBPRemoveAction);
    connect(mSoftBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeSoftBPActionSlot()));

    // Remove All
    mSoftBPRemoveAllAction = new QAction("Remove All", this);
    connect(mSoftBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllSoftBPActionSlot()));

    // Enable/Disable
    mSoftBPEnableDisableAction = new QAction("Enable", this);
    mSoftBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mSoftBPEnableDisableAction->setShortcut(QKeySequence(Qt::Key_Space));
    mSoftBPTable->addAction(mSoftBPEnableDisableAction);
    connect(mSoftBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableSoftBPActionSlot()));
}

void BreakpointsView::softwareBPContextMenuSlot(const QPoint & pos)
{
    if(mSoftBPTable->getRowCount() != 0)
    {
        int wI = 0;
        QMenu* wMenu = new QMenu(this);
        uint_t wVA = mSoftBPTable->getCellContent(mSoftBPTable->getInitialSelection(), 0).toULongLong(0, 16);
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

        // Separator
        wMenu->addSeparator();

        // Remove All
        wMenu->addAction(mSoftBPRemoveAllAction);

        QAction* wAction = wMenu->exec(mSoftBPTable->mapToGlobal(pos));
    }
}

void BreakpointsView::removeSoftBPActionSlot()
{
    uint_t wVA = mSoftBPTable->getCellContent(mSoftBPTable->getInitialSelection(), 0).toULongLong(0, 16);
    Breakpoints::removeBP(bp_normal, wVA);
}

void BreakpointsView::removeAllSoftBPActionSlot()
{

}

void BreakpointsView::enableDisableSoftBPActionSlot()
{
    Breakpoints::toogleBPByDisabling(bp_normal, mSoftBPTable->getCellContent(mSoftBPTable->getInitialSelection(), 0).toULongLong(0, 16));
}


/************************************************************************************
                         Memory Context Menu Management
************************************************************************************/
void BreakpointsView::setupMemBPRightClickContextMenu()
{
    // Remove
    mMemBPRemoveAction = new QAction("Remove", this);
    mMemBPRemoveAction->setShortcutContext(Qt::WidgetShortcut);
    mMemBPRemoveAction->setShortcut(QKeySequence(Qt::Key_Delete));
    mMemBPTable->addAction(mMemBPRemoveAction);
    connect(mMemBPRemoveAction, SIGNAL(triggered()), this, SLOT(removeMemBPActionSlot()));

    // Remove All
    mMemBPRemoveAllAction = new QAction("Remove All", this);
    connect(mMemBPRemoveAllAction, SIGNAL(triggered()), this, SLOT(removeAllMemBPActionSlot()));

    // Enable/Disable
    mMemBPEnableDisableAction = new QAction("Enable", this);
    mMemBPEnableDisableAction->setShortcutContext(Qt::WidgetShortcut);
    mMemBPEnableDisableAction->setShortcut(QKeySequence(Qt::Key_Space));
    mMemBPTable->addAction(mMemBPEnableDisableAction);
    connect(mMemBPEnableDisableAction, SIGNAL(triggered()), this, SLOT(enableDisableMemBPActionSlot()));
}

void BreakpointsView::memoryBPContextMenuSlot(const QPoint & pos)
{
    if(mMemBPTable->getRowCount() != 0)
    {
        int wI = 0;
        QMenu* wMenu = new QMenu(this);
        uint_t wVA = mMemBPTable->getCellContent(mMemBPTable->getInitialSelection(), 0).toULongLong(0, 16);
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

        // Separator
        wMenu->addSeparator();

        // Remove All
        wMenu->addAction(mMemBPRemoveAllAction);

        QAction* wAction = wMenu->exec(mMemBPTable->mapToGlobal(pos));
    }
}

void BreakpointsView::removeMemBPActionSlot()
{
    uint_t wVA = mMemBPTable->getCellContent(mMemBPTable->getInitialSelection(), 0).toULongLong(0, 16);
    Breakpoints::removeBP(bp_memory, wVA);
}

void BreakpointsView::removeAllMemBPActionSlot()
{

}

void BreakpointsView::enableDisableMemBPActionSlot()
{
    Breakpoints::toogleBPByDisabling(bp_memory, mMemBPTable->getCellContent(mMemBPTable->getInitialSelection(), 0).toULongLong(0, 16));
}

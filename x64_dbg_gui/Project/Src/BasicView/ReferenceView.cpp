#include "ReferenceView.h"
#include <QMessageBox>
#include "Configuration.h"
#include "Bridge.h"

ReferenceView::ReferenceView()
{
    // Setup SearchListView settings
    mSearchStartCol = 1;

    // Create search progress bar
    mSearchProgress = new QProgressBar();
    mSearchProgress->setRange(0, 100);
    mSearchProgress->setTextVisible(false);
    mSearchProgress->setMaximumHeight(15);

    // Add the progress bar to the main layout
    mMainLayout->addWidget(mSearchProgress);

    // Setup signals
    connect(Bridge::getBridge(), SIGNAL(referenceAddColumnAt(int, QString)), this, SLOT(addColumnAt(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(referenceSetRowCount(int_t)), this, SLOT(setRowCount(int_t)));
    connect(Bridge::getBridge(), SIGNAL(referenceSetCellContent(int, int, QString)), this, SLOT(setCellContent(int, int, QString)));
    connect(Bridge::getBridge(), SIGNAL(referenceReloadData()), this, SLOT(reloadData()));
    connect(Bridge::getBridge(), SIGNAL(referenceSetSingleSelection(int, bool)), this, SLOT(setSingleSelection(int, bool)));
    connect(Bridge::getBridge(), SIGNAL(referenceSetProgress(int)), mSearchProgress, SLOT(setValue(int)));
    connect(Bridge::getBridge(), SIGNAL(referenceSetSearchStartCol(int)), this, SLOT(setSearchStartCol(int)));
    connect(this, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(referenceContextMenu(QMenu*)));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followGenericAddress()));

    setupContextMenu();
}

void ReferenceView::setupContextMenu()
{
    mFollowAddress = new QAction("&Follow in Disassembler", this);
    connect(mFollowAddress, SIGNAL(triggered()), this, SLOT(followAddress()));

    mFollowDumpAddress = new QAction("Follow in &Dump", this);
    connect(mFollowDumpAddress, SIGNAL(triggered()), this, SLOT(followDumpAddress()));

    mToggleBreakpoint = new QAction("Toggle Breakpoint", this);
    mToggleBreakpoint->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mToggleBreakpoint);
    mList->addAction(mToggleBreakpoint);
    mSearchList->addAction(mToggleBreakpoint);
    connect(mToggleBreakpoint, SIGNAL(triggered()), this, SLOT(toggleBreakpoint()));

    mToggleBookmark = new QAction("Toggle Bookmark", this);
    mToggleBookmark->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mToggleBookmark);
    mList->addAction(mToggleBookmark);
    mSearchList->addAction(mToggleBookmark);
    connect(mToggleBookmark, SIGNAL(triggered()), this, SLOT(toggleBookmark()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void ReferenceView::disconnectBridge()
{
    disconnect(Bridge::getBridge(), SIGNAL(referenceAddColumnAt(int, QString)), this, SLOT(addColumnAt(int, QString)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetRowCount(int_t)), this, SLOT(setRowCount(int_t)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetCellContent(int, int, QString)), this, SLOT(setCellContent(int, int, QString)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceReloadData()), this, SLOT(reloadData()));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetSingleSelection(int, bool)), this, SLOT(setSingleSelection(int, bool)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetProgress(int)), mSearchProgress, SLOT(setValue(int)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetSearchStartCol(int)), this, SLOT(setSearchStartCol(int)));
}

void ReferenceView::refreshShortcutsSlot()
{
    mToggleBreakpoint->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mToggleBookmark->setShortcut(ConfigShortcut("ActionToggleBookmark"));
}

void ReferenceView::addColumnAt(int width, QString title)
{
    int charwidth = mList->getCharWidth();
    if(width)
        width = charwidth * width + 8;
    else
        width = 0;
    mSearchBox->setText("");
    if(title.toLower() == "&data&")
    {
        mFollowDumpDefault = true;
        title = "Data";
    }
    mList->addColumnAt(width, title, true);
    mSearchList->addColumnAt(width, title, true);
}

void ReferenceView::setRowCount(int_t count)
{
    mSearchBox->setText("");
    mList->setRowCount(count);
}

void ReferenceView::setCellContent(int r, int c, QString s)
{
    mSearchBox->setText("");
    mList->setCellContent(r, c, s);
}

void ReferenceView::reloadData()
{
    mSearchBox->setText("");
    mList->reloadData();
}

void ReferenceView::setSingleSelection(int index, bool scroll)
{
    mSearchBox->setText("");
    mList->setSingleSelection(index);
    if(scroll) //TODO: better scrolling
        mList->setTableOffset(index);
}

void ReferenceView::setSearchStartCol(int col)
{
    if(col < mList->getColumnCount())
        this->mSearchStartCol = col;
}

void ReferenceView::referenceContextMenu(QMenu* wMenu)
{
    if(!this->mCurList->getRowCount())
        return;
    const char* addrText = this->mCurList->getCellContent(this->mCurList->getInitialSelection(), 0).toUtf8().constData();
    if(!DbgIsValidExpression(addrText))
        return;
    uint_t addr = DbgValFromString(addrText);
    if(!DbgMemIsValidReadPtr(addr))
        return;
    wMenu->addAction(mFollowAddress);
    wMenu->addAction(mFollowDumpAddress);
    wMenu->addSeparator();
    wMenu->addAction(mToggleBreakpoint);
    wMenu->addAction(mToggleBookmark);
}

void ReferenceView::followAddress()
{
    DbgCmdExecDirect(QString("disasm " + this->mCurList->getCellContent(this->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

void ReferenceView::followDumpAddress()
{
    DbgCmdExecDirect(QString("dump " + this->mCurList->getCellContent(this->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

void ReferenceView::followGenericAddress()
{
    if(mFollowDumpDefault)
        followDumpAddress();
    else
        followAddress();
}

void ReferenceView::toggleBreakpoint()
{
    if(!DbgIsDebugging())
        return;

    if(!this->mCurList->getRowCount())
        return;
    const char* addrText = this->mCurList->getCellContent(this->mCurList->getInitialSelection(), 0).toUtf8().constData();
    if(!DbgIsValidExpression(addrText))
        return;
    uint_t wVA = DbgValFromString(addrText);
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
    {
        wCmd = "bc " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else
    {
        wCmd = "bp " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }

    DbgCmdExec(wCmd.toUtf8().constData());
    this->mSearchList->selectNext();
}

void ReferenceView::toggleBookmark()
{
    if(!DbgIsDebugging())
        return;

    if(!this->mCurList->getRowCount())
        return;
    const char* addrText = this->mCurList->getCellContent(this->mCurList->getInitialSelection(), 0).toUtf8().constData();
    if(!DbgIsValidExpression(addrText))
        return;
    uint_t wVA = DbgValFromString(addrText);
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    bool result;
    if(DbgGetBookmarkAt(wVA))
        result = DbgSetBookmarkAt(wVA, false);
    else
        result = DbgSetBookmarkAt(wVA, true);
    if(!result)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetBookmarkAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }
    GuiUpdateAllViews();
}

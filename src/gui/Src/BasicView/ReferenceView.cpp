#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QTabWidget>
#include "ReferenceView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "MiscUtil.h"

ReferenceView::ReferenceView(bool sourceView, QWidget* parent) : StdSearchListView(parent, true, false), mParent(dynamic_cast<QTabWidget*>(parent))
{
    // Setup SearchListView settings
    mSearchStartCol = 1;

    // Widget container for progress
    QWidget* progressWidget = new QWidget();

    // Create the layout for the progress bars
    QHBoxLayout* layoutProgress = new QHBoxLayout();
    progressWidget->setLayout(layoutProgress);
    layoutProgress->setContentsMargins(2, 0, 0, 0);
    layoutProgress->setSpacing(4);

    // Create current task search progress bar
    mSearchCurrentTaskProgress = new QProgressBar();
    mSearchCurrentTaskProgress->setRange(0, 100);
    mSearchCurrentTaskProgress->setTextVisible(true);
    mSearchCurrentTaskProgress->setMaximumHeight(15);
    layoutProgress->addWidget(mSearchCurrentTaskProgress);

    // Create total search progress bar
    mSearchTotalProgress = new QProgressBar();
    mSearchTotalProgress->setRange(0, 100);
    mSearchTotalProgress->setTextVisible(true);
    mSearchTotalProgress->setMaximumHeight(15);
    layoutProgress->addWidget(mSearchTotalProgress);

    // Label for the number of references
    mCountTotalLabel = new QLabel("");
    mCountTotalLabel->setAlignment(Qt::AlignCenter);
    mCountTotalLabel->setMaximumHeight(16);
    mCountTotalLabel->setMinimumWidth(40);
    mCountTotalLabel->setContentsMargins(2, 0, 5, 0);
    layoutProgress->addWidget(mCountTotalLabel);

    if(!sourceView)
    {
        // Add the progress bar and label to the main layout
        layout()->addWidget(progressWidget);
    }
    connect(this, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(referenceContextMenu(QMenu*)));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followGenericAddress()));

    setupContextMenu();
}

void ReferenceView::setupContextMenu()
{
    QIcon disassembler = DIcon(ArchValue("processor32.png", "processor64.png"));
    mFollowAddress = new QAction(disassembler, tr("&Follow in Disassembler"), this);
    connect(mFollowAddress, SIGNAL(triggered()), this, SLOT(followAddress()));

    mFollowDumpAddress = new QAction(DIcon("dump.png"), tr("Follow in &Dump"), this);
    connect(mFollowDumpAddress, SIGNAL(triggered()), this, SLOT(followDumpAddress()));

    mFollowApiAddress = new QAction(tr("Follow &API Address"), this);
    connect(mFollowApiAddress, SIGNAL(triggered()), this, SLOT(followApiAddress()));

    mToggleBreakpoint = new QAction(DIcon("breakpoint_toggle.png"), tr("Toggle Breakpoint"), this);
    mToggleBreakpoint->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(mToggleBreakpoint);
    StdSearchListView::addAction(mToggleBreakpoint);
    connect(mToggleBreakpoint, SIGNAL(triggered()), this, SLOT(toggleBreakpoint()));

    mToggleBookmark = new QAction(DIcon("bookmark_toggle.png"), tr("Toggle Bookmark"), this);
    mToggleBookmark->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(mToggleBookmark);
    StdSearchListView::addAction(mToggleBookmark);
    connect(mToggleBookmark, SIGNAL(triggered()), this, SLOT(toggleBookmark()));

    mSetBreakpointOnAllCommands = new QAction(DIcon("breakpoint_seton_all_commands.png"), tr("Set breakpoint on all commands"), this);
    connect(mSetBreakpointOnAllCommands, SIGNAL(triggered()), this, SLOT(setBreakpointOnAllCommands()));

    mRemoveBreakpointOnAllCommands = new QAction(DIcon("breakpoint_remove_all_commands.png"), tr("Remove breakpoint on all commands"), this);
    connect(mRemoveBreakpointOnAllCommands, SIGNAL(triggered()), this, SLOT(removeBreakpointOnAllCommands()));

    mSetBreakpointOnAllApiCalls = new QAction(tr("Set breakpoint on all api calls"), this);
    connect(mSetBreakpointOnAllApiCalls, SIGNAL(triggered()), this, SLOT(setBreakpointOnAllApiCalls()));

    mRemoveBreakpointOnAllApiCalls = new QAction(tr("Remove breakpoint on all api calls"), this);
    connect(mRemoveBreakpointOnAllApiCalls, SIGNAL(triggered()), this, SLOT(removeBreakpointOnAllApiCalls()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void ReferenceView::connectBridge()
{
    connect(Bridge::getBridge(), SIGNAL(referenceReloadData()), this, SLOT(reloadDataSlot()));
    connect(Bridge::getBridge(), SIGNAL(referenceSetSingleSelection(int, bool)), this, SLOT(setSingleSelection(int, bool)));
    connect(Bridge::getBridge(), SIGNAL(referenceSetProgress(int)), this, SLOT(referenceSetProgressSlot(int)));
    connect(Bridge::getBridge(), SIGNAL(referenceSetCurrentTaskProgress(int, QString)), this, SLOT(referenceSetCurrentTaskProgressSlot(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(referenceAddCommand(QString, QString)), this, SLOT(addCommand(QString, QString)));
    connect(stdSearchList(), SIGNAL(selectionChangedSignal(int)), this, SLOT(searchSelectionChanged(int)));
    connect(stdList(), SIGNAL(selectionChangedSignal(int)), this, SLOT(searchSelectionChanged(int)));
}

void ReferenceView::disconnectBridge()
{
    disconnect(Bridge::getBridge(), SIGNAL(referenceReloadData()), this, SLOT(reloadDataSlot()));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetSingleSelection(int, bool)), this, SLOT(setSingleSelection(int, bool)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetProgress(int)), this, SLOT(referenceSetProgressSlot(int)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetCurrentTaskProgress(int, QString)), this, SLOT(referenceSetCurrentTaskProgressSlot(int, QString)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceAddCommand(QString, QString)), this, SLOT(addCommand(QString, QString)));
    disconnect(stdSearchList(), SIGNAL(selectionChangedSignal(int)), this, SLOT(searchSelectionChanged(int)));
    disconnect(stdList(), SIGNAL(selectionChangedSignal(int)), this, SLOT(searchSelectionChanged(int)));
}

int ReferenceView::progress() const
{
    return mSearchTotalProgress->value();
}

int ReferenceView::currentTaskProgress() const
{
    return mSearchCurrentTaskProgress->value();
}

void ReferenceView::refreshShortcutsSlot()
{
    mToggleBreakpoint->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mToggleBookmark->setShortcut(ConfigShortcut("ActionToggleBookmark"));
}

void ReferenceView::referenceSetProgressSlot(int progress)
{
    mSearchTotalProgress->setValue(progress);
    mSearchTotalProgress->setAlignment(Qt::AlignCenter);
    mSearchTotalProgress->setFormat(tr("Total Progress %1%").arg(QString::number(progress)));
    mCountTotalLabel->setText(QString("%1").arg(stdList()->getRowCount()));
}

void ReferenceView::referenceSetCurrentTaskProgressSlot(int progress, QString taskTitle)
{
    mSearchCurrentTaskProgress->setValue(progress);
    mSearchCurrentTaskProgress->setAlignment(Qt::AlignCenter);
    mSearchCurrentTaskProgress->setFormat(taskTitle + " " + QString::number(progress) + "%");
}

void ReferenceView::searchSelectionChanged(int index)
{
    DbgValToString("$__disasm_refindex", index);
    DbgValToString("$__dump_refindex", index);
}

void ReferenceView::reloadDataSlot()
{
    if(mUpdateCountLabel)
    {
        mUpdateCountLabel = true;
        mCountTotalLabel->setText(QString("%1").arg(stdList()->getRowCount()));
    }
    reloadData();
}

void ReferenceView::addColumnAtRef(int width, QString title)
{
    int charwidth = getCharWidth();
    if(width)
        width = charwidth * width + 8;
    else
        width = 0;
    clearFilter();
    if(title.toLower() == "&data&")
        title = "Data";
    StdSearchListView::addColumnAt(width, title, true);
}

void ReferenceView::setRowCount(dsint count)
{
    if(!stdList()->getRowCount() && count) //from zero to N rows
        searchSelectionChanged(0);
    mUpdateCountLabel = true;
    StdSearchListView::setRowCount(count);
}

void ReferenceView::setSingleSelection(int index, bool scroll)
{
    clearFilter();
    stdList()->setSingleSelection(index);
    if(scroll) //TODO: better scrolling
        stdList()->setTableOffset(index);
}

void ReferenceView::addCommand(QString title, QString command)
{
    mCommandTitles.append(title);
    mCommands.append(command);
}

void ReferenceView::referenceContextMenu(QMenu* wMenu)
{
    if(!mCurList->getRowCount())
        return;
    QString text = mCurList->getCellContent(mCurList->getInitialSelection(), 0);
    duint addr;
    if(!DbgFunctions()->ValFromString(text.toUtf8().constData(), &addr))
        return;
    if(DbgMemIsValidReadPtr(addr))
    {
        wMenu->addAction(mFollowAddress);
        wMenu->addAction(mFollowDumpAddress);
        dsint apiaddr = apiAddressFromString(mCurList->getCellContent(mCurList->getInitialSelection(), 1));
        if(apiaddr)
            wMenu->addAction(mFollowApiAddress);
        wMenu->addSeparator();
        wMenu->addAction(mToggleBreakpoint);
        wMenu->addAction(mSetBreakpointOnAllCommands);
        wMenu->addAction(mRemoveBreakpointOnAllCommands);
        if(apiaddr)
        {
            char label[MAX_LABEL_SIZE] = "";
            if(DbgGetLabelAt(apiaddr, SEG_DEFAULT, label))
            {
                wMenu->addSeparator();
                mSetBreakpointOnAllApiCalls->setText(tr("Set breakpoint on all calls to %1").arg(label));
                wMenu->addAction(mSetBreakpointOnAllApiCalls);
                mRemoveBreakpointOnAllApiCalls->setText(tr("Remove breakpoint on all calls to %1").arg(label));
                wMenu->addAction(mRemoveBreakpointOnAllApiCalls);
            }
        }
        wMenu->addSeparator();
        wMenu->addAction(mToggleBookmark);
    }
    if(this->mCommands.size() > 0)
    {
        wMenu->addSeparator();
        for(auto i = 0; i < this->mCommandTitles.size(); i++)
        {
            QAction* newCommandAction = new QAction(this->mCommandTitles.at(i), wMenu);
            newCommandAction->setData(QVariant(mCommands.at(i)));
            connect(newCommandAction, SIGNAL(triggered()), this, SLOT(referenceExecCommand()));
            wMenu->addAction(newCommandAction);
        }
    }
}

void ReferenceView::followAddress()
{
    DbgCmdExecDirect(QString("disasm " + mCurList->getCellContent(mCurList->getInitialSelection(), 0)));
}

void ReferenceView::followDumpAddress()
{
    DbgCmdExecDirect(QString("dump " + mCurList->getCellContent(mCurList->getInitialSelection(), 0)));
}

void ReferenceView::followApiAddress()
{
    dsint apiValue = apiAddressFromString(mCurList->getCellContent(mCurList->getInitialSelection(), 1));
    DbgCmdExecDirect(QString("disasm " + ToPtrString(apiValue)));
}

void ReferenceView::followGenericAddress()
{
    auto addr = DbgValFromString(mCurList->getCellContent(mCurList->getInitialSelection(), 0).toUtf8().constData());
    if(!addr)
        return;
    if(DbgFunctions()->MemIsCodePage(addr, false))
        followAddress();
    else
    {
        followDumpAddress();
        emit Bridge::getBridge()->getDumpAttention();
    }
}

void ReferenceView::setBreakpointAt(int row, BPSetAction action)
{
    if(!DbgIsDebugging())
        return;

    if(!mCurList->getRowCount())
        return;
    QString addrText = mCurList->getCellContent(row, 0).toUtf8().constData();
    duint wVA;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &wVA))
        return;
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
    {
        if(action == Toggle || action == Remove)
            wCmd = "bc " + ToPtrString(wVA);
        else if(action == Disable)
            wCmd = "bpd " + ToPtrString(wVA);
        else if(action == Enable)
            wCmd = "bpe " + ToPtrString(wVA);
    }
    else if(wBpType == bp_none && (action == Toggle || action == Enable))
    {
        wCmd = "bp " + ToPtrString(wVA);
    }

    DbgCmdExecDirect(wCmd);
}

void ReferenceView::toggleBreakpoint()
{
    if(!DbgIsDebugging())
        return;

    if(!mCurList->getRowCount())
        return;

    setBreakpointAt(mCurList->getInitialSelection(), Toggle);
}

void ReferenceView::setBreakpointOnAllCommands()
{
    GuiUpdateDisable();
    for(int i = 0; i < mCurList->getRowCount(); i++)
        setBreakpointAt(i, Enable);
    GuiUpdateEnable(true);
}

void ReferenceView::removeBreakpointOnAllCommands()
{
    GuiUpdateDisable();
    for(int i = 0; i < mCurList->getRowCount(); i++)
        setBreakpointAt(i, Remove);
    GuiUpdateEnable(true);
}

void ReferenceView::setBreakpointOnAllApiCalls()
{
    if(!mCurList->getRowCount())
        return;
    dsint apiaddr = apiAddressFromString(mCurList->getCellContent(mCurList->getInitialSelection(), 1));
    if(!apiaddr)
        return;
    QString apiText = mCurList->getCellContent(mCurList->getInitialSelection(), 1);
    GuiUpdateDisable();
    for(int i = 0; i < mCurList->getRowCount(); i++)
        if(mCurList->getCellContent(i, 1) == apiText)
            setBreakpointAt(i, Enable);
    GuiUpdateEnable(true);
}

void ReferenceView::removeBreakpointOnAllApiCalls()
{
    if(!mCurList->getRowCount())
        return;

    dsint apiaddr = apiAddressFromString(mCurList->getCellContent(mCurList->getInitialSelection(), 1));
    if(!apiaddr)
        return;
    QString apiText = mCurList->getCellContent(mCurList->getInitialSelection(), 1);
    GuiUpdateDisable();
    for(int i = 0; i < mCurList->getRowCount(); i++)
        if(mCurList->getCellContent(i, 1) == apiText)
            setBreakpointAt(i, Remove);
    GuiUpdateEnable(true);
}

void ReferenceView::toggleBookmark()
{
    if(!DbgIsDebugging())
        return;

    if(!mCurList->getRowCount())
        return;
    QString addrText = mCurList->getCellContent(mCurList->getInitialSelection(), 0);
    duint wVA;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &wVA))
        return;
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    bool result;
    if(DbgGetBookmarkAt(wVA))
        result = DbgSetBookmarkAt(wVA, false);
    else
        result = DbgSetBookmarkAt(wVA, true);
    if(!result)
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetBookmarkAt failed!"));
    GuiUpdateAllViews();
}

dsint ReferenceView::apiAddressFromString(const QString & s)
{
    QRegExp regEx("call.+<(.+)>");
    regEx.indexIn(s);
    QStringList list = regEx.capturedTexts();
    if(list.length() < 2)
        return 0;
    QString match = list[1];
    if(match[0] == QChar('&'))
        match.remove(0, 1);
    duint value;
    return DbgFunctions()->ValFromString(match.toUtf8().constData(), &value) && DbgMemIsValidReadPtr(value) ? value : 0;
}

void ReferenceView::referenceExecCommand()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if(act != nullptr)
    {
        QString command = act->data().toString();
        for(int selected : mCurList->getSelection()) //to do: enable multi-selection
        {
            QString specializedCommand = command;
            for(int i = 0; i < mCurList->getColumnCount(); i++)
            {
                QString token = "$" + QString::number(i);
                if(specializedCommand.contains(token))
                    specializedCommand.replace(token, mCurList->getCellContent(selected, i));
            }
            DbgCmdExec(specializedCommand);
        }
    }
}

void ReferenceView::mouseReleaseEvent(QMouseEvent* event)
{
    if(mParent)
    {
        if(event->button() == Qt::ForwardButton)
            mParent->setCurrentIndex(std::min(mParent->currentIndex() + 1, mParent->count()));
        else if(event->button() == Qt::BackButton)
            mParent->setCurrentIndex(std::max(mParent->currentIndex() - 1, 0));
    }
}

bool ReferenceView::eventFilter(QObject* obj, QEvent* event)
{
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if(Qt::ControlModifier == keyEvent->modifiers() && Qt::Key_C == keyEvent->key())
    {
        if(mCurList->getRowCount())
        {
            mCurList->copyLineSlot();
        }
    }
    return QWidget::eventFilter(obj, event);
}

#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QTabWidget>
#include "ReferenceView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "MiscUtil.h"
#include "DisassemblyPopup.h"

ReferenceView::ReferenceView(bool sourceView, QWidget* parent) : StdSearchListView(parent, true, false), mParent(dynamic_cast<QTabWidget*>(parent))
{
    // Setup SearchListView settings
    mSearchStartCol = 1;
    enableMultiSelection(true);

    // Widget container for progress
    QWidget* progressWidget = new QWidget(this);

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

    // Add disassembly popups
    new DisassemblyPopup(stdList(), Bridge::getArchitecture());
    new DisassemblyPopup(stdSearchList(), Bridge::getArchitecture());
}

void ReferenceView::setupContextMenu()
{
    QIcon disassembler = DIcon(ArchValue("processor32", "processor64"));
    mFollowAddress = new QAction(disassembler, tr("&Follow in Disassembler"), this);
    connect(mFollowAddress, SIGNAL(triggered()), this, SLOT(followAddress()));

    mFollowDumpAddress = new QAction(DIcon("dump"), tr("Follow in &Dump"), this);
    connect(mFollowDumpAddress, SIGNAL(triggered()), this, SLOT(followDumpAddress()));

    mFollowApiAddress = new QAction(tr("Follow &API Address"), this);
    connect(mFollowApiAddress, SIGNAL(triggered()), this, SLOT(followApiAddress()));

    mToggleBreakpoint = new QAction(DIcon("breakpoint_toggle"), tr("Toggle Breakpoint"), this);
    mToggleBreakpoint->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(mToggleBreakpoint);
    StdSearchListView::addAction(mToggleBreakpoint);
    connect(mToggleBreakpoint, SIGNAL(triggered()), this, SLOT(toggleBreakpoint()));

    mToggleBookmark = new QAction(DIcon("bookmark_toggle"), tr("Toggle Bookmark"), this);
    mToggleBookmark->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(mToggleBookmark);
    StdSearchListView::addAction(mToggleBookmark);
    connect(mToggleBookmark, SIGNAL(triggered()), this, SLOT(toggleBookmark()));

    mSetBreakpointOnAllCommands = new QAction(DIcon("breakpoint_seton_all_commands"), tr("Set breakpoint on all commands"), this);
    connect(mSetBreakpointOnAllCommands, SIGNAL(triggered()), this, SLOT(setBreakpointOnAllCommands()));

    mRemoveBreakpointOnAllCommands = new QAction(DIcon("breakpoint_remove_all_commands"), tr("Remove breakpoint on all commands"), this);
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
    connect(this, SIGNAL(selectionChanged(duint)), this, SLOT(searchSelectionChanged(duint)));
}

void ReferenceView::disconnectBridge()
{
    disconnect(Bridge::getBridge(), SIGNAL(referenceReloadData()), this, SLOT(reloadDataSlot()));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetSingleSelection(int, bool)), this, SLOT(setSingleSelection(int, bool)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetProgress(int)), this, SLOT(referenceSetProgressSlot(int)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceSetCurrentTaskProgress(int, QString)), this, SLOT(referenceSetCurrentTaskProgressSlot(int, QString)));
    disconnect(Bridge::getBridge(), SIGNAL(referenceAddCommand(QString, QString)), this, SLOT(addCommand(QString, QString)));
    disconnect(this, SIGNAL(selectionChanged(duint)), this, SLOT(searchSelectionChanged(duint)));
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

void ReferenceView::searchSelectionChanged(duint index)
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

void ReferenceView::setRowCount(duint count)
{
    if(!stdList()->getRowCount() && count) //from zero to N rows
        searchSelectionChanged(0);
    mUpdateCountLabel = true;
    StdSearchListView::setRowCount(count);
}

void ReferenceView::setSingleSelection(int index, bool scroll)
{
    //clearFilter();
    stdList()->setSingleSelection(index);
    stdSearchList()->setSingleSelection(index);
    if(scroll) //TODO: better scrolling
    {
        stdList()->setTableOffset(index);
        stdSearchList()->setTableOffset(index);
    }
}

void ReferenceView::addCommand(QString title, QString command)
{
    mCommandTitles.append(title);
    mCommands.append(command);
}

void ReferenceView::referenceContextMenu(QMenu* menu)
{
    if(!mCurList->getRowCount())
        return;
    QString text = mCurList->getCellContent(mCurList->getInitialSelection(), 0);
    duint addr;
    if(!DbgFunctions()->ValFromString(text.toUtf8().constData(), &addr))
        return;
    if(DbgMemIsValidReadPtr(addr))
    {
        menu->addAction(mFollowAddress);
        menu->addAction(mFollowDumpAddress);
        dsint apiaddr = apiAddressFromString(mCurList->getCellContent(mCurList->getInitialSelection(), 1));
        if(apiaddr)
            menu->addAction(mFollowApiAddress);
        menu->addSeparator();
        menu->addAction(mToggleBreakpoint);
        menu->addAction(mSetBreakpointOnAllCommands);
        menu->addAction(mRemoveBreakpointOnAllCommands);
        if(apiaddr)
        {
            char label[MAX_LABEL_SIZE] = "";
            if(DbgGetLabelAt(apiaddr, SEG_DEFAULT, label))
            {
                menu->addSeparator();
                mSetBreakpointOnAllApiCalls->setText(tr("Set breakpoint on all calls to %1").arg(label));
                menu->addAction(mSetBreakpointOnAllApiCalls);
                mRemoveBreakpointOnAllApiCalls->setText(tr("Remove breakpoint on all calls to %1").arg(label));
                menu->addAction(mRemoveBreakpointOnAllApiCalls);
            }
        }
        menu->addSeparator();
        menu->addAction(mToggleBookmark);
    }
    if(this->mCommands.size() > 0)
    {
        menu->addSeparator();
        for(auto i = 0; i < this->mCommandTitles.size(); i++)
        {
            QAction* newCommandAction = new QAction(this->mCommandTitles.at(i), menu);
            newCommandAction->setData(QVariant(mCommands.at(i)));
            connect(newCommandAction, SIGNAL(triggered()), this, SLOT(referenceExecCommand()));
            menu->addAction(newCommandAction);
        }
    }
}

void ReferenceView::followAddress()
{
    auto index = mCurList->getInitialSelection();
    searchSelectionChanged(index);
    DbgCmdExecDirect(QString("disasm " + mCurList->getCellContent(index, 0)));
}

void ReferenceView::followDumpAddress()
{
    auto index = mCurList->getInitialSelection();
    searchSelectionChanged(index);
    DbgCmdExecDirect(QString("dump " + mCurList->getCellContent(index, 0)));
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

void ReferenceView::setBreakpointAt(duint row, BPSetAction action)
{
    if(!DbgIsDebugging())
        return;

    if(!mCurList->getRowCount())
        return;
    QString addrText = mCurList->getCellContent(row, 0).toUtf8().constData();
    duint va = 0;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &va))
        return;
    if(!DbgMemIsValidReadPtr(va))
        return;

    BPXTYPE bpType = DbgGetBpxTypeAt(va);
    QString cmd;

    if((bpType & bp_normal) == bp_normal)
    {
        if(action == Toggle || action == Remove)
            cmd = "bc " + ToPtrString(va);
        else if(action == Disable)
            cmd = "bpd " + ToPtrString(va);
        else if(action == Enable)
            cmd = "bpe " + ToPtrString(va);
    }
    else if(bpType == bp_none && (action == Toggle || action == Enable))
    {
        cmd = "bp " + ToPtrString(va);
    }

    DbgCmdExecDirect(cmd);
}

void ReferenceView::toggleBreakpoint()
{
    if(!DbgIsDebugging())
        return;

    if(!mCurList->getRowCount())
        return;

    GuiDisableUpdateScope s;
    foreach(int i, mCurList->getSelection())
        setBreakpointAt(i, Toggle);
}

void ReferenceView::setBreakpointOnAllCommands()
{
    GuiDisableUpdateScope s;
    for(duint i = 0; i < mCurList->getRowCount(); i++)
        setBreakpointAt(i, Enable);
}

void ReferenceView::removeBreakpointOnAllCommands()
{
    GuiDisableUpdateScope s;
    for(duint i = 0; i < mCurList->getRowCount(); i++)
        setBreakpointAt(i, Remove);
}

void ReferenceView::setBreakpointOnAllApiCalls()
{
    if(!mCurList->getRowCount())
        return;
    dsint apiaddr = apiAddressFromString(mCurList->getCellContent(mCurList->getInitialSelection(), 1));
    if(!apiaddr)
        return;
    QString apiText = mCurList->getCellContent(mCurList->getInitialSelection(), 1);

    GuiDisableUpdateScope s;
    for(duint i = 0; i < mCurList->getRowCount(); i++)
        if(mCurList->getCellContent(i, 1) == apiText)
            setBreakpointAt(i, Enable);
}

void ReferenceView::removeBreakpointOnAllApiCalls()
{
    if(!mCurList->getRowCount())
        return;

    dsint apiaddr = apiAddressFromString(mCurList->getCellContent(mCurList->getInitialSelection(), 1));
    if(!apiaddr)
        return;
    QString apiText = mCurList->getCellContent(mCurList->getInitialSelection(), 1);

    GuiDisableUpdateScope s;
    for(duint i = 0; i < mCurList->getRowCount(); i++)
        if(mCurList->getCellContent(i, 1) == apiText)
            setBreakpointAt(i, Remove);
}

void ReferenceView::toggleBookmark()
{
    if(!DbgIsDebugging())
        return;

    if(!mCurList->getRowCount())
        return;
    QString addrText = mCurList->getCellContent(mCurList->getInitialSelection(), 0);
    duint va = 0;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &va))
        return;
    if(!DbgMemIsValidReadPtr(va))
        return;

    bool result;
    if(DbgGetBookmarkAt(va))
        result = DbgSetBookmarkAt(va, false);
    else
        result = DbgSetBookmarkAt(va, true);
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
            for(duint i = 0; i < mCurList->getColumnCount(); i++)
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

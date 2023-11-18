#include "CallStackView.h"
#include "CommonActions.h"
#include "Bridge.h"
#include "DisassemblyPopup.h"

CallStackView::CallStackView(StdTable* parent) : StdIconTable(parent)
{
    int charwidth = getCharWidth();

    addColumnAt(32 * charwidth, tr("Thread ID"), false);
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("Address"), false); //address in the stack
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("To"), false); //return to
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("From"), false); //return from
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("Size"), false); //size
    addColumnAt(9 * charwidth, tr("Party"), false); //party
    addColumnAt(50 * charwidth, tr("Comment"), false);
    setIconColumn(ColParty);
    loadColumnFromConfig("CallStack");

    connect(Bridge::getBridge(), SIGNAL(updateCallStack()), this, SLOT(updateCallStackSlot()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(followFromSlot()));

    setupContextMenu();

    new DisassemblyPopup(this, Bridge::getArchitecture());
}

void CallStackView::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });
    mCommonActions = new CommonActions(this, getActionHelperFuncs(), [this]()
    {
        return getSelectionVa();
    });
    QIcon icon = DIcon(ArchValue("processor32", "processor64"));
    mMenuBuilder->addAction(makeAction(icon, tr("Follow &Address"), SLOT(followAddressSlot())), [this](QMenu*)
    {
        return isSelectionValid();
    });
    mMenuBuilder->addAction(makeAction(icon, tr("Follow &To"), SLOT(followToSlot())), [this](QMenu*)
    {
        return isSelectionValid();
    });
    QAction* mFollowFrom = mMenuBuilder->addAction(makeAction(icon, tr("Follow &From"), SLOT(followFromSlot())), [this](QMenu*)
    {
        return !getCellContent(getInitialSelection(), ColFrom).isEmpty() && isSelectionValid();
    });
    mFollowFrom->setShortcutContext(Qt::WidgetShortcut);
    mFollowFrom->setShortcut(QKeySequence("enter"));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followFromSlot()));
    // Breakpoint menu
    // TODO: Is Label/Comment/Bookmark useful?
    mCommonActions->build(mMenuBuilder, CommonActions::ActionBreakpoint);
    mMenuBuilder->addSeparator();

    QAction* showSuspectedCallStack = makeAction(tr("Show Suspected Call Stack Frame"), SLOT(showSuspectedCallStackSlot()));
    mMenuBuilder->addAction(showSuspectedCallStack, [showSuspectedCallStack](QMenu*)
    {
        duint i;
        if(!BridgeSettingGetUint("Engine", "ShowSuspectedCallStack", &i))
            i = 0;
        if(i != 0)
            showSuspectedCallStack->setText(tr("Show Active Call Stack Frame"));
        else
            showSuspectedCallStack->setText(tr("Show Suspected Call Stack Frame"));
        return true;
    });

    mMenuBuilder->addSeparator();
    QAction* followInThreads = makeAction(tr("Follow in Threads"), SLOT(followInThreadsSlot()));
    mMenuBuilder->addAction(followInThreads, [this](QMenu*)
    {
        return isThreadHeaderSelected();
    });

    QAction* renameThread = makeAction(tr("Rename Thread"), SLOT(renameThreadSlot()));
    mMenuBuilder->addAction(renameThread, [this](QMenu*)
    {
        return isThreadHeaderSelected();
    });

    MenuBuilder* mCopyMenu = new MenuBuilder(this);
    setupCopyMenu(mCopyMenu);
    // Column count cannot be zero
    mMenuBuilder->addSeparator();
    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), mCopyMenu);
    mMenuBuilder->loadFromConfig();
}

QString CallStackView::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    if(isSelected(row))
        painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));

    bool isSpaceRow = !getCellContent(row, ColThread).isEmpty();

    if(col == ColThread && row == 0)
    {
        QString ret = getCellContent(row, col);
        if(!ret.isEmpty())
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("ThreadCurrentBackgroundColor")));
            painter->setPen(QPen(ConfigColor("ThreadCurrentColor")));
            painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        }
        return "";
    }
    else if(col > ColThread && isSpaceRow)
    {
        auto mid = h / 2.0;
        painter->drawLine(QPointF(x, y + mid), QPointF(x + w, y + mid));
    }
    else if(col == ColFrom || col == ColTo || col == ColAddress)
    {
        QString ret = getCellContent(row, col);
        BPXTYPE bpxtype = DbgGetBpxTypeAt(getCellUserdata(row, col));
        if(bpxtype & bp_normal)
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor")));
            painter->setPen(QPen(ConfigColor("DisassemblyBreakpointColor")));
            painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, ret);
            return "";
        }
        else if(bpxtype & bp_hardware)
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyHardwareBreakpointBackgroundColor")));
            painter->setPen(QPen(ConfigColor("DisassemblyHardwareBreakpointColor")));
            painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, ret);
            return "";
        }
    }
    return StdIconTable::paintContent(painter, row, col, x, y, w, h);
}

void CallStackView::updateCallStackSlot()
{
    if(!DbgFunctions()->GetCallStackByThread)
        return;

    THREADLIST threadList;
    memset(&threadList, 0, sizeof(THREADLIST));
    DbgGetThreadList(&threadList);

    int currentRow = 0;
    int currentIndexToDraw = 0;
    setRowCount(0);
    for(int j = 0; j < threadList.count; j++)
    {
        if(!j)
            currentIndexToDraw = threadList.CurrentThread; // Draw the current thread first
        else if(j == threadList.CurrentThread)
            currentIndexToDraw = 0; // Draw the previously skipped thread
        else
            currentIndexToDraw = j;

        DBGCALLSTACK callstack;
        memset(&callstack, 0, sizeof(DBGCALLSTACK));
        DbgFunctions()->GetCallStackByThread(threadList.list[currentIndexToDraw].BasicInfo.Handle, &callstack);
        setRowCount(currentRow + callstack.total + 1);

        QString threadName = threadList.list[currentIndexToDraw].BasicInfo.threadName;
        QString colThreadString = ToDecString(threadList.list[currentIndexToDraw].BasicInfo.ThreadId);

        if(threadName.size() > 0)
            colThreadString += " - " + threadName; // The " - " is crucial here, because later split is happening

        setCellContent(currentRow, ColThread, colThreadString);

        currentRow++;

        for(int i = 0; i < callstack.total; i++, currentRow++)
        {
            QString addrText = ToPtrString(callstack.entries[i].addr);
            setCellContent(currentRow, ColAddress, addrText);
            addrText = ToPtrString(callstack.entries[i].to);
            setCellContent(currentRow, ColTo, addrText);
            if(callstack.entries[i].from)
            {
                addrText = ToPtrString(callstack.entries[i].from);
                setCellContent(currentRow, ColFrom, addrText);
            }
            setCellUserdata(currentRow, ColFrom, callstack.entries[i].from);
            setCellUserdata(currentRow, ColTo, callstack.entries[i].to);
            setCellUserdata(currentRow, ColAddress, callstack.entries[i].addr);
            if(i != callstack.total - 1)
                setCellContent(currentRow, ColSize, ToHexString(callstack.entries[i + 1].addr - callstack.entries[i].addr));
            else
                setCellContent(currentRow, ColSize, "");
            setCellContent(currentRow, ColComment, callstack.entries[i].comment);
            int party = DbgFunctions()->ModGetParty(callstack.entries[i].to);
            switch(party)
            {
            case mod_user:
                setCellContent(currentRow, ColParty, tr("User"));
                setRowIcon(currentRow, DIcon("markasuser"));
                break;
            case mod_system:
                setCellContent(currentRow, ColParty, tr("System"));
                setRowIcon(currentRow, DIcon("markassystem"));
                break;
            default:
                setCellContent(currentRow, ColParty, QString::number(party));
                setRowIcon(currentRow, DIcon("markasparty"));
                break;
            }
        }
        if(callstack.total)
            BridgeFree(callstack.entries);

    }

    if(threadList.count)
        BridgeFree(threadList.list);

    reloadData();
}

void CallStackView::contextMenuSlot(const QPoint pos)
{
    QMenu menu(this); //create context menu
    mMenuBuilder->build(&menu);
    if(!menu.isEmpty())
        menu.exec(mapToGlobal(pos)); //execute context menu
}

void CallStackView::followAddressSlot()
{
    QString addrText = getCellContent(getInitialSelection(), ColAddress);
    DbgCmdExecDirect(QString("sdump " + addrText));
}

void CallStackView::followToSlot()
{
    QString addrText = getCellContent(getInitialSelection(), ColTo);
    DbgCmdExecDirect(QString("disasm " + addrText));
}

void CallStackView::followFromSlot()
{
    QString addrText = getCellContent(getInitialSelection(), ColFrom);
    DbgCmdExecDirect(QString("disasm " + addrText));
}

void CallStackView::renameThreadSlot()
{
    QStringList split = getCellContent(getInitialSelection(), 0).split(" - ", QString::SplitBehavior::SkipEmptyParts);
    duint threadId = split[0].toInt();
    QString threadName = split.length() > 1 ? split[1] : "";
    if(!SimpleInputBox(this, tr("Thread name - %1").arg(threadId), threadName, threadName, QString()))
        return;

    DbgCmdExec(QString("setthreadname %1, \"%2\"").arg(ToHexString(threadId)).arg(DbgCmdEscape(threadName)));
}
void CallStackView::followInThreadsSlot()
{
    QStringList threadIDName = getCellContent(getInitialSelection(), ColThread).split(" - ");
    if(threadIDName[0].size() == 0)
        return;

    DbgCmdExecDirect(QString("showthreadid " + threadIDName[0]));
}

void CallStackView::showSuspectedCallStackSlot()
{
    duint i;
    if(!BridgeSettingGetUint("Engine", "ShowSuspectedCallStack", &i))
        i = 0;
    i = (i == 0) ? 1 : 0;
    BridgeSettingSetUint("Engine", "ShowSuspectedCallStack", i);
    DbgSettingsUpdated();
    updateCallStackSlot();
    emit Bridge::getBridge()->updateDump();
}

bool CallStackView::isThreadHeaderSelected()
{
    return !getCellContent(getInitialSelection(), ColThread).isEmpty();
}

bool CallStackView::isSelectionValid()
{
    return getCellContent(getInitialSelection(), ColThread).isEmpty();
}

// For breakpoint/run to selection
duint CallStackView::getSelectionVa()
{
    if(isSelectionValid())
        return getCellUserdata(getInitialSelection(), ColFrom);
    else
        return 0;
}

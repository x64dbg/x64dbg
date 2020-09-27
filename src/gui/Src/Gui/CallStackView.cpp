#include "CallStackView.h"
#include "Bridge.h"

CallStackView::CallStackView(StdTable* parent) : StdTable(parent)
{
    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("Address"), true); //address in the stack
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("To"), false); //return to
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("From"), false); //return from
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("Size"), false); //size
    addColumnAt(50 * charwidth, tr("Comment"), false);
    addColumnAt(8 * charwidth, tr("Party"), false); //party
    loadColumnFromConfig("CallStack");

    connect(Bridge::getBridge(), SIGNAL(updateCallStack()), this, SLOT(updateCallStack()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(followFrom()));

    setupContextMenu();
}

void CallStackView::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });
    QIcon icon = DIcon(ArchValue("processor32.png", "processor64.png"));
    mMenuBuilder->addAction(makeAction(icon, tr("Follow &Address"), SLOT(followAddress())));
    mMenuBuilder->addAction(makeAction(icon, tr("Follow &To"), SLOT(followTo())));
    QAction* mFollowFrom = mMenuBuilder->addAction(makeAction(icon, tr("Follow &From"), SLOT(followFrom())), [this](QMenu*)
    {
        return !getCellContent(getInitialSelection(), 2).isEmpty();
    });
    mFollowFrom->setShortcutContext(Qt::WidgetShortcut);
    mFollowFrom->setShortcut(QKeySequence("enter"));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followFrom()));
    mMenuBuilder->addSeparator();
    QAction* wShowSuspectedCallStack = makeShortcutAction(tr("Show Suspected Call Stack Frame"), SLOT(showSuspectedCallStack()), "space");
    mMenuBuilder->addAction(wShowSuspectedCallStack, [wShowSuspectedCallStack](QMenu*)
    {
        duint i;
        if(!BridgeSettingGetUint("Engine", "ShowSuspectedCallStack", &i))
            i = 0;
        if(i != 0)
            wShowSuspectedCallStack->setText(tr("Show Active Call Stack Frame"));
        else
            wShowSuspectedCallStack->setText(tr("Show Suspected Call Stack Frame"));
        return true;
    });
    MenuBuilder* mCopyMenu = new MenuBuilder(this);
    setupCopyMenu(mCopyMenu);
    // Column count cannot be zero
    mMenuBuilder->addSeparator();
    mMenuBuilder->addMenu(makeMenu(DIcon("copy.png"), tr("&Copy")), mCopyMenu);
    mMenuBuilder->loadFromConfig();
}

void CallStackView::updateCallStack()
{
    DBGCALLSTACK callstack;
    memset(&callstack, 0, sizeof(DBGCALLSTACK));
    if(!DbgFunctions()->GetCallStack)
        return;
    DbgFunctions()->GetCallStack(&callstack);
    setRowCount(callstack.total);
    for(int i = 0; i < callstack.total; i++)
    {
        QString addrText = ToPtrString(callstack.entries[i].addr);
        setCellContent(i, 0, addrText);
        addrText = ToPtrString(callstack.entries[i].to);
        setCellContent(i, 1, addrText);
        if(callstack.entries[i].from)
        {
            addrText = ToPtrString(callstack.entries[i].from);
            setCellContent(i, 2, addrText);
        }
        if(i != callstack.total - 1)
            setCellContent(i, 3, ToHexString(callstack.entries[i + 1].addr - callstack.entries[i].addr));
        else
            setCellContent(i, 3, "");
        setCellContent(i, 4, callstack.entries[i].comment);
        int party = DbgFunctions()->ModGetParty(callstack.entries[i].to);
        switch(party)
        {
        case mod_user:
            setCellContent(i, 5, tr("User"));
            break;
        case mod_system:
            setCellContent(i, 5, tr("System"));
            break;
        default:
            setCellContent(i, 5, QString::number(party));
            break;
        }
    }
    if(callstack.total)
        BridgeFree(callstack.entries);
    reloadData();
}

void CallStackView::contextMenuSlot(const QPoint pos)
{
    QMenu wMenu(this); //create context menu
    mMenuBuilder->build(&wMenu);
    if(!wMenu.isEmpty())
        wMenu.exec(mapToGlobal(pos)); //execute context menu
}

void CallStackView::followAddress()
{
    QString addrText = getCellContent(getInitialSelection(), 0);
    DbgCmdExecDirect(QString("sdump " + addrText).toUtf8().constData());
}

void CallStackView::followTo()
{
    QString addrText = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
}

void CallStackView::followFrom()
{
    QString addrText = getCellContent(getInitialSelection(), 2);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
}

void CallStackView::showSuspectedCallStack()
{
    duint i;
    if(!BridgeSettingGetUint("Engine", "ShowSuspectedCallStack", &i))
        i = 0;
    i = (i == 0) ? 1 : 0;
    BridgeSettingSetUint("Engine", "ShowSuspectedCallStack", i);
    DbgSettingsUpdated();
    updateCallStack();
    emit Bridge::getBridge()->updateDump();
}

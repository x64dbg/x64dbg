#include "CallStackView.h"
#include "Bridge.h"

CallStackView::CallStackView(StdTable* parent) : StdTable(parent)
{
    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("Address"), true); //address in the stack
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("To"), true); //return to
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("From"), true); //return from
    addColumnAt(0, tr("Comment"), true);
    loadColumnFromConfig("CallStack");

    connect(Bridge::getBridge(), SIGNAL(updateCallStack()), this, SLOT(updateCallStack()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(followTo()));

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
    QAction* mFollowTo = mMenuBuilder->addAction(makeAction(icon, tr("Follow &To"), SLOT(followTo())));
    mFollowTo->setShortcutContext(Qt::WidgetShortcut);
    mFollowTo->setShortcut(QKeySequence("enter"));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followTo()));
    mMenuBuilder->addAction(makeAction(icon, tr("Follow &From"), SLOT(followFrom())), [this](QMenu*)
    {
        return !getCellContent(getInitialSelection(), 2).isEmpty();
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
        setCellContent(i, 3, callstack.entries[i].comment);
    }
    if(callstack.total)
        BridgeFree(callstack.entries);
    reloadData();
}

void CallStackView::contextMenuSlot(const QPoint pos)
{
    QMenu wMenu(this); //create context menu
    mMenuBuilder->build(&wMenu);
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

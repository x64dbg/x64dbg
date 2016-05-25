#include "CallStackView.h"
#include "Bridge.h"

CallStackView::CallStackView(StdTable* parent) : StdTable(parent)
{
    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("Address"), true); //address in the stack
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("To"), true); //return to
    addColumnAt(8 + charwidth * sizeof(dsint) * 2, tr("From"), true); //return from
    addColumnAt(0, tr("Comment"), true);

    connect(Bridge::getBridge(), SIGNAL(updateCallStack()), this, SLOT(updateCallStack()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));

    setupContextMenu();
}

void CallStackView::setupContextMenu()
{
    mFollowAddress = new QAction(tr("Follow &Address"), this);
    connect(mFollowAddress, SIGNAL(triggered()), this, SLOT(followAddress()));
    mFollowTo = new QAction(tr("Follow &To"), this);
    mFollowTo->setShortcutContext(Qt::WidgetShortcut);
    mFollowTo->setShortcut(QKeySequence("enter"));
    connect(mFollowTo, SIGNAL(triggered()), this, SLOT(followTo()));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followTo()));
    mFollowFrom = new QAction(tr("Follow &From"), this);
    connect(mFollowFrom, SIGNAL(triggered()), this, SLOT(followFrom()));
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
        QString addrText = QString("%1").arg((duint)callstack.entries[i].addr, sizeof(duint) * 2, 16, QChar('0')).toUpper();
        setCellContent(i, 0, addrText);
        addrText = QString("%1").arg((duint)callstack.entries[i].to, sizeof(duint) * 2, 16, QChar('0')).toUpper();
        setCellContent(i, 1, addrText);
        if(callstack.entries[i].from)
        {
            addrText = QString("%1").arg((duint)callstack.entries[i].from, sizeof(duint) * 2, 16, QChar('0')).toUpper();
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
    if(!DbgIsDebugging())
        return;
    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addAction(mFollowAddress);
    wMenu->addAction(mFollowTo);
    QString wStr = getCellContent(getInitialSelection(), 2);
    if(wStr.length())
        wMenu->addAction(mFollowFrom);
    QMenu wCopyMenu("&Copy", this);
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
    }
    wMenu->exec(mapToGlobal(pos)); //execute context menu
}

void CallStackView::doubleClickedSlot()
{
    followTo();
}

void CallStackView::followAddress()
{
    QString addrText = getCellContent(getInitialSelection(), 0);
    DbgCmdExecDirect(QString("sdump " + addrText).toUtf8().constData());
    emit showCpu();
}

void CallStackView::followTo()
{
    QString addrText = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    emit showCpu();
}

void CallStackView::followFrom()
{
    QString addrText = getCellContent(getInitialSelection(), 2);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    emit showCpu();
}

#include "SEHChainView.h"
#include "Bridge.h"

SEHChainView::SEHChainView(StdTable* parent) : StdTable(parent)
{
    int charWidth = getCharWidth();

    addColumnAt(8 + charWidth * sizeof(dsint) * 2, tr("Address"), true); //address in the stack
    addColumnAt(8 + charWidth * sizeof(dsint) * 2, tr("Handler"), true); // Exception Handler
    addColumnAt(8 + charWidth * 50, tr("Module/Label"), false);
    addColumnAt(charWidth * 10, tr("Comment"), false);
    connect(Bridge::getBridge(), SIGNAL(updateSEHChain()), this, SLOT(updateSEHChain()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    loadColumnFromConfig("SEH");
    setupContextMenu();
}

void SEHChainView::setupContextMenu()
{
    mFollowAddress = new QAction(tr("Follow &Address"), this);
    connect(mFollowAddress, SIGNAL(triggered()), this, SLOT(followAddress()));
    mFollowHandler = new QAction(tr("Follow Handler"), this);
    mFollowHandler->setShortcutContext(Qt::WidgetShortcut);
    mFollowHandler->setShortcut(QKeySequence("enter"));
    connect(mFollowHandler, SIGNAL(triggered()), this, SLOT(followHandler()));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followHandler()));
}

void SEHChainView::updateSEHChain()
{
    DBGSEHCHAIN sehchain;
    memset(&sehchain, 0, sizeof(DBGSEHCHAIN));
    if(!DbgFunctions()->GetSEHChain)
        return;
    DbgFunctions()->GetSEHChain(&sehchain);
    setRowCount(sehchain.total);
    for(duint i = 0; i < sehchain.total; i++)
    {
        QString cellText = QString("%1").arg(sehchain.records[i].addr, sizeof(duint) * 2, 16, QChar('0')).toUpper();
        setCellContent(i, 0, cellText);
        cellText = QString("%1").arg(sehchain.records[i].handler, sizeof(duint) * 2, 16, QChar('0')).toUpper();
        setCellContent(i, 1, cellText);

        char label[MAX_LABEL_SIZE] = "";
        char module[MAX_MODULE_SIZE] = "";
        DbgGetModuleAt(sehchain.records[i].handler, module);
        QString label_text;
        if(DbgGetLabelAt(sehchain.records[i].handler, SEG_DEFAULT, label))
            label_text = "<" + QString(module) + "." + QString(label) + ">";
        else
            label_text = QString(module);
        setCellContent(i, 2, label_text);
        QString comment;
        if(GetCommentFormat(sehchain.records[i].handler, comment))
            setCellContent(i, 3, comment);
    }
    if(sehchain.total)
        BridgeFree(sehchain.records);
    reloadData();
}

void SEHChainView::contextMenuSlot(const QPoint pos)
{
    if(!DbgIsDebugging())
        return;
    QMenu wMenu(this); //create context menu
    wMenu.addAction(mFollowAddress);
    wMenu.addAction(mFollowHandler);
    QMenu wCopyMenu(tr("&Copy"), this);
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
    }
    wMenu.exec(mapToGlobal(pos)); //execute context menu
}

void SEHChainView::doubleClickedSlot()
{
    followHandler();
}

void SEHChainView::followAddress()
{
    QString addrText = getCellContent(getInitialSelection(), 0);
    DbgCmdExecDirect(QString("sdump " + addrText).toUtf8().constData());
}

void SEHChainView::followHandler()
{
    QString addrText = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
}

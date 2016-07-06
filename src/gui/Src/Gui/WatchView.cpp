#include "Bridge.h"
#include "WatchView.h"
#include "CPUMultiDump.h"
#include "MiscUtil.h"

WatchView::WatchView(CPUMultiDump* parent) : StdTable(parent)
{
    int charWidth = getCharWidth();
    addColumnAt(8 + charWidth * 12, tr("Name"), false);
    addColumnAt(8 + charWidth * 20, tr("Expression"), false);
    addColumnAt(8 + charWidth * sizeof(duint) * 2, tr("Value"), false);
    addColumnAt(8 + charWidth * 8, tr("Type"), false);
    addColumnAt(150, tr("Watchdog Mode"), false);
    addColumnAt(30, tr("ID"), false);

    connect(Bridge::getBridge(), SIGNAL(updateWatch()), this, SLOT(updateWatch()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    updateColors();
    setupContextMenu();
    setDrawDebugOnly(true);
}

void WatchView::updateWatch()
{
    if(!DbgIsDebugging())
    {
        setRowCount(0);
        return;
    }
    BridgeList<WATCHINFO> WatchList;
    DbgGetWatchList(&WatchList);
    setRowCount(WatchList.Count());
    for(int i = 0; i < WatchList.Count(); i++)
    {
        setCellContent(i, 0, QString::fromUtf8(WatchList[i].WatchName));
        setCellContent(i, 1, QString::fromUtf8(WatchList[i].Expression));
        setCellContent(i, 2, ToPtrString(WatchList[i].value));
        switch(WatchList[i].varType)
        {
        case WATCHVARTYPE::TYPE_UINT:
            setCellContent(i, 3, "UINT");
            break;
        case WATCHVARTYPE::TYPE_INT:
            setCellContent(i, 3, "INT");
            break;
        case WATCHVARTYPE::TYPE_FLOAT:
            setCellContent(i, 3, "FLOAT");
            break;
        case WATCHVARTYPE::TYPE_INVALID:
        default:
            setCellContent(i, 3, "INVALID");
            break;
        }
        switch(WatchList[i].watchdogMode)
        {
        case WATCHDOGMODE::MODE_DISABLED:
        default:
            setCellContent(i, 4, tr("Disabled"));
            break;
        case WATCHDOGMODE::MODE_CHANGED:
            setCellContent(i, 4, tr("Changed"));
            break;
        case WATCHDOGMODE::MODE_ISTRUE:
            setCellContent(i, 4, tr("Is true"));
            break;
        case WATCHDOGMODE::MODE_ISFALSE:
            setCellContent(i, 4, tr("Is false"));
            break;
        case WATCHDOGMODE::MODE_UNCHANGED:
            setCellContent(i, 4, tr("Not changed"));
            break;
        }
        setCellContent(i, 5, QString::number(WatchList[i].id));
    }
    reloadData();
}

void WatchView::updateColors()
{
    mWatchTriggeredColor = QPen(ConfigColor("WatchTriggeredColor"));
    mWatchTriggeredBackgroundColor = QBrush(ConfigColor("WatchTriggeredBackgroundColor"));
    StdTable::updateColors();
}

void WatchView::setupContextMenu()
{
    mMenu = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });
    mMenu->addAction(makeAction(tr("&Add..."), SLOT(addWatchSlot())));
    mMenu->addAction(makeAction(tr("&Delete"), SLOT(delWatchSlot())));
    mMenu->addAction(makeAction(tr("Rename"), SLOT(renameWatchSlot())));
    mMenu->addAction(makeAction(tr("&Edit..."), SLOT(editWatchSlot())));
    QMenu* watchdogMenu = new QMenu(tr("Watchdog"), this);
    watchdogMenu->addAction(makeAction(QIcon(":/icons/images/close-all-tabs.png"), tr("Disabled"), SLOT(watchdogDisableSlot())));
    watchdogMenu->addSeparator();
    watchdogMenu->addAction(makeAction(tr("Changed"), SLOT(watchdogChangedSlot())));
    watchdogMenu->addAction(makeAction(tr("Not changed"), SLOT(watchdogUnchangedSlot())));
    watchdogMenu->addAction(makeAction(tr("Is true"), SLOT(watchdogIsTrueSlot())));
    watchdogMenu->addAction(makeAction(tr("Is false"), SLOT(watchdogIsFalseSlot())));
    mMenu->addMenu(watchdogMenu);
}

QString WatchView::getSelectedId()
{
    return getCellContent(getInitialSelection(), 5);
}

QString WatchView::paintContent(QPainter *painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    const dsint row = rowBase + rowOffset;
    if(row != getInitialSelection() && DbgFunctions()->WatchIsWatchdogTriggered(getCellContent(row, 5).toUInt()))
    {
        painter->fillRect(QRect(x, y, w, h), mWatchTriggeredBackgroundColor);
        painter->setPen(mWatchTriggeredColor); //white text
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        return "";
    }
    else
        return ret;
}

//SLOTS

void WatchView::contextMenuSlot(const QPoint & pos)
{
    QMenu wMenu(this);
    mMenu->build(&wMenu);
    wMenu.exec(mapToGlobal(pos));
}

void WatchView::addWatchSlot()
{
    QString name;
    if(SimpleInputBox(this, tr("Enter the expression to watch"), "", name))
        DbgCmdExecDirect(QString("AddWatch ").append(name).toUtf8().constData());
    updateWatch();
}

void WatchView::delWatchSlot()
{
    DbgCmdExecDirect(QString("DelWatch ").append(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::renameWatchSlot()
{
    QString name;
    if(SimpleInputBox(this, tr("Enter the name of the watch variable"), getCellContent(getInitialSelection(), 0), name))
        DbgCmdExecDirect(QString("SetWatchName ").append(getSelectedId() + "," + name).toUtf8().constData());
    updateWatch();
}

void WatchView::editWatchSlot()
{
    QString expr;
    if(SimpleInputBox(this, tr("Enter the expression to watch"), "", expr))
        DbgCmdExecDirect(QString("SetWatchExpression ").append(getSelectedId()).append(",").append(expr).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogDisableSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"disabled\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogChangedSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"changed\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogUnchangedSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"unchanged\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogIsTrueSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"istrue\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

void WatchView::watchdogIsFalseSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"isfalse\"").arg(getSelectedId()).toUtf8().constData());
    updateWatch();
}

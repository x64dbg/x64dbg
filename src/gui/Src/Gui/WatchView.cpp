#include "Bridge.h"
#include "WatchView.h"
#include "CPUMultiDump.h"
#include "WordEditDialog.h"
#include "MiscUtil.h"

WatchView::WatchView(CPUMultiDump* parent) : StdTable(parent)
{
    int charWidth = getCharWidth();
    addColumnAt(8 + charWidth * 12, tr("Name"), true);
    addColumnAt(8 + charWidth * 20, tr("Expression"), true);
    addColumnAt(8 + charWidth * sizeof(duint) * 2, tr("Value"), true);
    addColumnAt(8 + charWidth * 8, tr("Type"), true);
    addColumnAt(150, tr("Watchdog Mode"), true);
    addColumnAt(30, tr("ID"), true, "", SortBy::AsInt);

    connect(Bridge::getBridge(), SIGNAL(updateWatch()), this, SLOT(updateWatch()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    setupContextMenu();
    setDrawDebugOnly(true);
    Initialize();
}

void WatchView::updateWatch()
{
    if(!DbgIsDebugging())
    {
        setRowCount(0);
        setSingleSelection(0);
        return;
    }
    BridgeList<WATCHINFO> WatchList;
    DbgGetWatchList(&WatchList);
    setRowCount(WatchList.Count());
    if(getInitialSelection() >= WatchList.Count() && WatchList.Count() > 0)
        setSingleSelection(WatchList.Count() - 1);
    for(int i = 0; i < WatchList.Count(); i++)
    {
        setCellContent(i, ColName, QString(WatchList[i].WatchName));
        setCellContent(i, ColExpr, QString(WatchList[i].Expression));
        switch(WatchList[i].varType)
        {
        case WATCHVARTYPE::TYPE_UINT:
            setCellContent(i, ColType, "UINT");
            setCellContent(i, ColValue, ToPtrString(WatchList[i].value));
            break;
        case WATCHVARTYPE::TYPE_INT:
            setCellContent(i, ColType, "INT");
            setCellContent(i, ColValue, QString::number((dsint)WatchList[i].value));
            break;
        case WATCHVARTYPE::TYPE_FLOAT:
            setCellContent(i, ColType, "FLOAT");
            setCellContent(i, ColValue, ToFloatString(&WatchList[i].value));
            break;
        case WATCHVARTYPE::TYPE_ASCII:
            setCellContent(i, ColType, "ASCII");
            {
                char buffer[128];
                // zero the buffer
                memset(buffer, 0, sizeof(buffer));
                if(DbgMemRead(WatchList[i].value, (unsigned char*)buffer, sizeof(buffer) - 1))
                {
                    // convert the ASCII string to QString
                    QString text = QString::fromLocal8Bit(buffer);
                    if(strlen(buffer) == sizeof(buffer) - 1)
                        text.append("...");
                    // remove CRLF
                    text.replace(QChar('\x13'), "\\r");
                    text.replace(QChar('\x10'), "\\n");
                    setCellContent(i, ColValue, text);
                }
                else
                    setCellContent(i, ColValue, tr("%1 is not readable.").arg(ToPtrString(WatchList[i].value)));
            }
            break;
        case WATCHVARTYPE::TYPE_UNICODE:
            setCellContent(i, ColType, "UNICODE");
            {
                unsigned short buffer[128];
                // zero the buffer
                memset(buffer, 0, sizeof(buffer));
                if(DbgMemRead(WatchList[i].value, (unsigned char*)buffer, sizeof(buffer) - sizeof(unsigned short)))
                {
                    QString text = QString::fromUtf16(buffer);
                    size_t size = text.size();
                    // Check if the last character is an incomplete UTF-16 surrogate.
                    if(text.at(text.size() - 1).isHighSurrogate())
                        text.chop(text.size() - 1); // Delete the incomplete surrogate.
                    // Check if something is truncated.
                    if(size == sizeof(buffer) / sizeof(unsigned short) - 1)
                        text.append("...");
                    // remove CRLF
                    text.replace(QChar('\x13'), "\\r");
                    text.replace(QChar('\x10'), "\\n");
                    setCellContent(i, ColValue, text);
                }
                else
                    setCellContent(i, ColValue, tr("%1 is not readable.").arg(ToPtrString(WatchList[i].value)));
            }
            break;
        case WATCHVARTYPE::TYPE_INVALID:
        default:
            setCellContent(i, ColType, "INVALID");
            setCellContent(i, ColValue, "");
            break;
        }
        switch(WatchList[i].watchdogMode)
        {
        case WATCHDOGMODE::MODE_DISABLED:
        default:
            setCellContent(i, ColWatchdog, tr("Disabled"));
            break;
        case WATCHDOGMODE::MODE_CHANGED:
            setCellContent(i, ColWatchdog, tr("Changed"));
            break;
        case WATCHDOGMODE::MODE_ISTRUE:
            setCellContent(i, ColWatchdog, tr("Is true"));
            break;
        case WATCHDOGMODE::MODE_ISFALSE:
            setCellContent(i, ColWatchdog, tr("Is false"));
            break;
        case WATCHDOGMODE::MODE_UNCHANGED:
            setCellContent(i, ColWatchdog, tr("Not changed"));
            break;
        }
        setCellContent(i, ColId, QString::number(WatchList[i].id));
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
    const auto & nonEmptyFunc = [this](QMenu*)
    {
        return getRowCount() != 0;
    };
    mMenu->addAction(makeAction(tr("&Add..."), SLOT(addWatchSlot())));
    mMenu->addAction(makeShortcutAction(tr("&Delete"), SLOT(delWatchSlot()), "ActionDeleteBreakpoint"), nonEmptyFunc);
    mMenu->addAction(makeAction(DIcon("labels"), tr("Rename"), SLOT(renameWatchSlot())), nonEmptyFunc);
    mMenu->addAction(makeAction(DIcon("modify"), tr("&Edit..."), SLOT(editWatchSlot())), nonEmptyFunc);
    mMenu->addAction(makeAction(DIcon("modify"), tr("&Modify..."), SLOT(modifyWatchSlot())), nonEmptyFunc);
    MenuBuilder* watchdogBuilder = new MenuBuilder(this, nonEmptyFunc);
    QMenu* watchdogMenu = new QMenu(tr("Watchdog"), this);
    watchdogMenu->setIcon(DIcon("animal-dog"));
    watchdogBuilder->addAction(makeAction(DIcon("disable"), tr("Disabled"), SLOT(watchdogDisableSlot())));
    watchdogBuilder->addSeparator();
    watchdogBuilder->addAction(makeAction(DIcon("arrow-restart"), tr("Changed"), SLOT(watchdogChangedSlot())));
    watchdogBuilder->addAction(makeAction(DIcon("control-pause"), tr("Not changed"), SLOT(watchdogUnchangedSlot())));
    watchdogBuilder->addAction(makeAction(DIcon("treat_selection_as_tbyte"), tr("Is true"), SLOT(watchdogIsTrueSlot()))); // TODO: better icon
    watchdogBuilder->addAction(makeAction(DIcon("treat_selection_as_fword"), tr("Is false"), SLOT(watchdogIsFalseSlot())));
    mMenu->addMenu(watchdogMenu, watchdogBuilder);
    MenuBuilder* typeBuilder = new MenuBuilder(this, nonEmptyFunc);
    QMenu* typeMenu = new QMenu(tr("Type"), this);
    typeBuilder->addAction(makeAction(DIcon("integer"), tr("Uint"), SLOT(setTypeUintSlot())));
    typeBuilder->addAction(makeAction(DIcon("integer"), tr("Int"), SLOT(setTypeIntSlot())));
    typeBuilder->addAction(makeAction(DIcon("float"), tr("Float"), SLOT(setTypeFloatSlot())));
    typeBuilder->addAction(makeAction(DIcon("ascii"), tr("Ascii"), SLOT(setTypeAsciiSlot())));
    typeBuilder->addAction(makeAction(DIcon("ascii-extended"), tr("Unicode"), SLOT(setTypeUnicodeSlot())));
    mMenu->addMenu(typeMenu, typeBuilder);
    mMenu->addSeparator();
    MenuBuilder* copyMenu = new MenuBuilder(this);
    setupCopyMenu(copyMenu);
    mMenu->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);
    mMenu->loadFromConfig();
}

QString WatchView::getSelectedId()
{
    return QChar('.') + getCellContent(getInitialSelection(), ColId);
}

QString WatchView::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, row, col, x, y, w, h);
    if(row != getInitialSelection() && DbgFunctions()->WatchIsWatchdogTriggered(getCellContent(row, ColId).toUInt()))
    {
        painter->fillRect(QRect(x, y, w, h), mWatchTriggeredBackgroundColor);
        painter->setPen(mWatchTriggeredColor); //white text
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        return QString();
    }
    else
        return ret;
}

//SLOTS

void WatchView::contextMenuSlot(const QPoint & pos)
{
    QMenu menu(this);
    mMenu->build(&menu);
    menu.exec(mapToGlobal(pos));
}

void WatchView::addWatchSlot()
{
    QString name;
    if(SimpleInputBox(this, tr("Enter the expression to watch"), "", name, tr("Example: [EAX]")))
        DbgCmdExecDirect(QString("AddWatch ").append(name));
    updateWatch();
}

void WatchView::delWatchSlot()
{
    DbgCmdExecDirect(QString("DelWatch ").append(getSelectedId()));
    updateWatch();
}

void WatchView::renameWatchSlot()
{
    QString name;
    QString originalName = getCellContent(getInitialSelection(), ColName);
    if(SimpleInputBox(this, tr("Enter the name of the watch variable"), originalName, name, originalName))
        DbgCmdExecDirect(QString("SetWatchName ").append(getSelectedId() + "," + name));
    updateWatch();
}

void WatchView::modifyWatchSlot()
{
    BridgeList<WATCHINFO> WatchList;
    DbgGetWatchList(&WatchList);
    auto sel = getInitialSelection();
    if(sel > WatchList.Count())
        return;
    WordEditDialog modifyDialog(this);
    modifyDialog.setup(tr("Modify \"%1\"").arg(QString(WatchList[sel].WatchName)), WatchList[sel].value, sizeof(duint));
    if(modifyDialog.exec() == QDialog::Accepted)
    {
        if(!DbgValToString(WatchList[sel].Expression, modifyDialog.getVal()))
            SimpleErrorBox(this, tr("Cannot modify \"%1\"").arg(QString(WatchList[sel].WatchName)), tr("It might not possible to assign a value to \"%1\".").arg(QString(WatchList[sel].Expression)));
    }
    updateWatch();
}

void WatchView::editWatchSlot()
{
    QString expr;
    QString originalExpr = getCellContent(getInitialSelection(), ColExpr);
    QString currentType = getCellContent(getInitialSelection(), ColType);
    if(SimpleInputBox(this, tr("Enter the expression to watch"), originalExpr, expr, tr("Example: [EAX]")))
        DbgCmdExecDirect(QString("SetWatchExpression ").append(getSelectedId()).append(",").append(expr).append(",").append(currentType));
    updateWatch();
}

void WatchView::watchdogDisableSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"disabled\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::watchdogChangedSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"changed\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::watchdogUnchangedSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"unchanged\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::watchdogIsTrueSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"istrue\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::watchdogIsFalseSlot()
{
    DbgCmdExecDirect(QString("SetWatchdog %1, \"isfalse\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::setTypeUintSlot()
{
    DbgCmdExecDirect(QString("SetWatchType %1, \"uint\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::setTypeIntSlot()
{
    DbgCmdExecDirect(QString("SetWatchType %1, \"int\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::setTypeFloatSlot()
{
    DbgCmdExecDirect(QString("SetWatchType %1, \"float\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::setTypeAsciiSlot()
{
    DbgCmdExecDirect(QString("SetWatchType %1, \"ascii\"").arg(getSelectedId()));
    updateWatch();
}

void WatchView::setTypeUnicodeSlot()
{
    DbgCmdExecDirect(QString("SetWatchType %1, \"unicode\"").arg(getSelectedId()));
    updateWatch();
}

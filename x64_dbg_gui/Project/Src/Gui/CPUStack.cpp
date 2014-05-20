#include "CPUStack.h"

CPUStack::CPUStack(QWidget *parent) : HexDump(parent)
{
    setShowHeader(false);
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //void*
    wColDesc.itemCount = 1;
#ifdef _WIN64
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
#else
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
#endif
    appendDescriptor(8+charwidth*2*sizeof(uint_t), "void*", false, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "Comments", false, wColDesc);

    connect(Bridge::getBridge(), SIGNAL(stackDumpAt(uint_t,uint_t)), this, SLOT(stackDumpAt(uint_t,uint_t)));
    connect(Bridge::getBridge(), SIGNAL(selectionStackGet(SELECTIONDATA*)), this, SLOT(selectionGet(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionStackSet(const SELECTIONDATA*)), this, SLOT(selectionSet(const SELECTIONDATA*)));

    setupContextMenu();

    mGoto = 0;
}

void CPUStack::setupContextMenu()
{
#ifdef _WIN64
    mGotoSp = new QAction("Follow R&SP", this);
    mGotoBp = new QAction("Follow R&BP", this);
#else
    mGotoSp = new QAction("Follow E&SP", this);
    mGotoBp = new QAction("Follow E&BP", this);
#endif //_WIN64
    mGotoSp->setShortcutContext(Qt::WidgetShortcut);
    mGotoSp->setShortcut(QKeySequence("*"));
    this->addAction(mGotoSp);
    connect(mGotoSp, SIGNAL(triggered()), this, SLOT(gotoSpSlot()));
    connect(mGotoBp, SIGNAL(triggered()), this, SLOT(gotoBpSlot()));

    mGotoExpression = new QAction("&Expression", this);
    mGotoExpression->setShortcutContext(Qt::WidgetShortcut);
    mGotoExpression->setShortcut(QKeySequence("ctrl+g"));
    this->addAction(mGotoExpression);
    connect(mGotoExpression, SIGNAL(triggered()), this, SLOT(gotoExpressionSlot()));

    mFollowDisasm = new QAction("&Follow in Disassembler", this);
    mFollowDisasm->setShortcutContext(Qt::WidgetShortcut);
    mFollowDisasm->setShortcut(QKeySequence("enter"));
    this->addAction(mFollowDisasm);
    connect(mFollowDisasm, SIGNAL(triggered()), this, SLOT(followDisasmSlot()));

    mFollowDump = new QAction("Follow in &Dump", this);
    connect(mFollowDump, SIGNAL(triggered()), this, SLOT(followDumpSlot()));

    mFollowStack = new QAction("Follow in &Stack", this);
    connect(mFollowStack, SIGNAL(triggered()), this, SLOT(followStackSlot()));
}

QString CPUStack::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString wStr=HexDump::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);

    // Compute RVA
    int wBytePerRowCount = getBytePerRowCount();
    int_t wRva = (rowBase + rowOffset) * wBytePerRowCount - mByteOffset;
    uint_t wVa = wRva + mMemPage->getBase();

    bool wIsSelected=isSelected(wRva);
    if(wIsSelected) //highlight if selected
        painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#C0C0C0")));

    bool wActiveStack=true;
    if(wVa<mCsp) //inactive stack
        wActiveStack=false;

    STACK_COMMENT comment;

    if(col == 0) // paint stack address
    {
        painter->save();
        if(wVa==mCsp) //CSP
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#000000")));
            painter->setPen(QPen(QColor("#FFFBF0")));
        }
        else if(wIsSelected)
            painter->setPen(QPen(QColor("#000000"))); //black address
        else
            painter->setPen(QPen(QColor("#808080")));
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, wStr);
        painter->restore();
        wStr = "";
    }
    else if(mDescriptor.at(col - 1).isData == true) //paint stack data
    {
        painter->save();
        if(wActiveStack)
            painter->setPen(QPen(QColor("#000000")));
        else
            painter->setPen(QPen(QColor("#808080")));
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, wStr);
        painter->restore();
        wStr = "";
    }
    else if(DbgStackCommentGet(mMemPage->getBase()+wRva, &comment)) //paint stack comments
    {
        wStr = QString(comment.comment);
        painter->save();
        if(wActiveStack)
        {
            if(*comment.color)
                painter->setPen(QPen(QColor(QString(comment.color))));
            else
                painter->setPen(QPen(QColor("#000000")));
        }
        else
            painter->setPen(QPen(QColor("#808080")));
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, wStr);
        painter->restore();
        wStr = "";
    }
    return wStr;
}

void CPUStack::contextMenuEvent(QContextMenuEvent* event)
{
    if(!DbgIsDebugging())
        return;

    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addAction(mGotoSp);
    wMenu->addAction(mGotoBp);
    wMenu->addAction(mGotoExpression);

    int_t selectedVa = getInitialSelection() + mMemPage->getBase();
    uint_t selectedData;
    if(DbgMemRead(selectedVa, (unsigned char*)&selectedData, sizeof(uint_t)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            uint_t stackBegin = mMemPage->getBase();
            uint_t stackEnd = stackBegin + mMemPage->getSize();
            if(selectedData >= stackBegin && selectedData < stackEnd)
                wMenu->addAction(mFollowStack);
            else
                wMenu->addAction(mFollowDisasm);
            wMenu->addAction(mFollowDump);
        }

    wMenu->exec(event->globalPos());
}

void CPUStack::stackDumpAt(uint_t addr, uint_t csp)
{
    mCsp=csp;
    printDumpAt(addr);
}

void CPUStack::gotoSpSlot()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExec("sdump csp");
}

void CPUStack::gotoBpSlot()
{
#ifdef _WIN64
    DbgCmdExec("sdump rbp");
#else
    DbgCmdExec("sdump ebp");
#endif //_WIN64
}

void CPUStack::gotoExpressionSlot()
{
    if(!DbgIsDebugging())
        return;
    uint_t size=0;
    uint_t base=DbgMemFindBaseAddr(mCsp, &size);
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->validRangeStart=base;
    mGoto->validRangeEnd=base+size;
    mGoto->setWindowTitle("Enter expression to follow in Stack...");
    if(mGoto->exec()==QDialog::Accepted)
    {
        QString cmd;
        DbgCmdExec(cmd.sprintf("sdump \"%s\"", mGoto->expressionText.toUtf8().constData()).toUtf8().constData());
    }
}

void CPUStack::selectionGet(SELECTIONDATA* selection)
{
    selection->start=getSelectionStart() + mBase;
    selection->end=getSelectionEnd() + mBase;
    Bridge::getBridge()->BridgeSetResult(1);
}

void CPUStack::selectionSet(const SELECTIONDATA* selection)
{
    int_t selMin=mBase;
    int_t selMax=selMin + mSize;
    int_t start=selection->start;
    int_t end=selection->end;
    if(start < selMin || start >= selMax || end < selMin || end >= selMax) //selection out of range
    {
        Bridge::getBridge()->BridgeSetResult(0);
        return;
    }
    setSingleSelection(start - selMin);
    expandSelectionUpTo(end - selMin);
    reloadData();
    Bridge::getBridge()->BridgeSetResult(1);
}

void CPUStack::followDisasmSlot()
{
    int_t selectedVa = getInitialSelection() + mMemPage->getBase();
    uint_t selectedData;
    if(DbgMemRead(selectedVa, (unsigned char*)&selectedData, sizeof(uint_t)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText=QString("%1").arg(selectedData, sizeof(int_t)*2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("disasm " + addrText).toUtf8().constData());
        }
}

void CPUStack::followDumpSlot()
{
    int_t selectedVa = getInitialSelection() + mMemPage->getBase();
    uint_t selectedData;
    if(DbgMemRead(selectedVa, (unsigned char*)&selectedData, sizeof(uint_t)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText=QString("%1").arg(selectedData, sizeof(int_t)*2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("dump " + addrText).toUtf8().constData());
        }
}

void CPUStack::followStackSlot()
{
    int_t selectedVa = getInitialSelection() + mMemPage->getBase();
    uint_t selectedData;
    if(DbgMemRead(selectedVa, (unsigned char*)&selectedData, sizeof(uint_t)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText=QString("%1").arg(selectedData, sizeof(int_t)*2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("sdump " + addrText).toUtf8().constData());
        }
}

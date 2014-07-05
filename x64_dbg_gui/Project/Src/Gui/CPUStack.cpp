#include "CPUStack.h"
#include "Configuration.h"
#include "HexEditDialog.h"

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

    backgroundColor=ConfigColor("StackBackgroundColor");
    textColor=ConfigColor("StackTextColor");
    selectionColor=ConfigColor("StackSelectionColor");
}

void CPUStack::colorsUpdated()
{
    HexDump::colorsUpdated();
    backgroundColor=ConfigColor("StackBackgroundColor");
    textColor=ConfigColor("StackTextColor");
    selectionColor=ConfigColor("StackSelectionColor");
}

void CPUStack::setupContextMenu()
{
    //Binary menu
    mBinaryMenu = new QMenu("B&inary", this);

    //Binary->Edit
    mBinaryEditAction = new QAction("&Edit", this);
    mBinaryEditAction->setShortcutContext(Qt::WidgetShortcut);
    mBinaryEditAction->setShortcut(QKeySequence("ctrl+e"));
    this->addAction(mBinaryEditAction);
    connect(mBinaryEditAction, SIGNAL(triggered()), this, SLOT(binaryEditSlot()));
    mBinaryMenu->addAction(mBinaryEditAction);

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
    // Compute RVA
    int wBytePerRowCount = getBytePerRowCount();
    int_t wRva = (rowBase + rowOffset) * wBytePerRowCount - mByteOffset;
    uint_t wVa = rvaToVa(wRva);

    bool wIsSelected=isSelected(wRva);
    if(wIsSelected) //highlight if selected
        painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor));

    bool wActiveStack=true;
    if(wVa<mCsp) //inactive stack
        wActiveStack=false;

    STACK_COMMENT comment;

    if(col == 0) // paint stack address
    {
        char label[MAX_LABEL_SIZE]="";
        QString addrText="";
        int_t curAddr = rvaToVa((rowBase + rowOffset) * getBytePerRowCount() - mByteOffset);
        addrText = QString("%1").arg(curAddr, sizeof(int_t)*2, 16, QChar('0')).toUpper();
        if(DbgGetLabelAt(curAddr, SEG_DEFAULT, label)) //has label
        {
            char module[MAX_MODULE_SIZE]="";
            if(DbgGetModuleAt(curAddr, module) && !QString(label).startsWith("JMP.&"))
                addrText+=" <"+QString(module)+"."+QString(label)+">";
            else
                addrText+=" <"+QString(label)+">";
        }
        else
            *label=0;
        QColor background;
        if(*label) //label
        {
            if(wVa==mCsp) //CSP
            {
                background=ConfigColor("StackCspBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackCspColor")));
            }
            else //no CSP
            {
                background=ConfigColor("StackLabelBackgroundColor");
                painter->setPen(ConfigColor("StackLabelColor"));
            }
        }
        else //no label
        {
            if(wVa==mCsp) //CSP
            {
                background=ConfigColor("StackCspBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackCspColor")));
            }
            else if(wIsSelected) //selected normal address
            {
                background=ConfigColor("StackSelectedAddressBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackSelectedAddressColor"))); //black address (DisassemblySelectedAddressColor)
            }
            else //normal address
            {
                background=ConfigColor("StackAddressBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackAddressColor")));
            }
        }
        if(background.alpha())
            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background when defined
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, addrText);
    }
    else if(mDescriptor.at(col - 1).isData == true) //paint stack data
    {
        QString wStr=HexDump::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
        if(wActiveStack)
            painter->setPen(QPen(textColor));
        else
            painter->setPen(QPen(ConfigColor("StackInactiveTextColor")));
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, wStr);
    }
    else if(DbgStackCommentGet(rvaToVa(wRva), &comment)) //paint stack comments
    {
        QString wStr = QString(comment.comment);
        if(wActiveStack)
        {
            if(*comment.color)
                painter->setPen(QPen(QColor(QString(comment.color))));
            else
                painter->setPen(QPen(textColor));
        }
        else
            painter->setPen(QPen(ConfigColor("StackInactiveTextColor")));
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, wStr);
    }
    return "";
}

void CPUStack::contextMenuEvent(QContextMenuEvent* event)
{
    if(!DbgIsDebugging())
        return;

    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addMenu(mBinaryMenu);
    wMenu->addAction(mGotoSp);
    wMenu->addAction(mGotoBp);
    wMenu->addAction(mGotoExpression);

    uint_t selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(uint_t)))
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
    selection->start=rvaToVa(getSelectionStart());
    selection->end=rvaToVa(getSelectionEnd());
    Bridge::getBridge()->BridgeSetResult(1);
}

void CPUStack::selectionSet(const SELECTIONDATA* selection)
{
    int_t selMin=mMemPage->getBase();
    int_t selMax=selMin + mMemPage->getSize();
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
    uint_t selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(uint_t)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText=QString("%1").arg(selectedData, sizeof(int_t)*2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("disasm " + addrText).toUtf8().constData());
        }
}

void CPUStack::followDumpSlot()
{
    uint_t selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(uint_t)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText=QString("%1").arg(selectedData, sizeof(int_t)*2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("dump " + addrText).toUtf8().constData());
        }
}

void CPUStack::followStackSlot()
{
    uint_t selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(uint_t)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText=QString("%1").arg(selectedData, sizeof(int_t)*2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("sdump " + addrText).toUtf8().constData());
        }
}

void CPUStack::binaryEditSlot()
{
    HexEditDialog hexEdit(this);
    int_t selStart = getSelectionStart();
    int_t selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.setWindowTitle("Edit data at " + QString("%1").arg(rvaToVa(selStart), sizeof(int_t) * 2, 16, QChar('0')).toUpper());
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    int_t dataSize = hexEdit.mHexEdit->data().size();
    int_t newSize = selSize > dataSize ? selSize : dataSize;
    data = new byte_t[newSize];
    mMemPage->read(data, selStart, newSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, newSize));
    mMemPage->write(patched.constData(), selStart, patched.size());
    reloadData();
}

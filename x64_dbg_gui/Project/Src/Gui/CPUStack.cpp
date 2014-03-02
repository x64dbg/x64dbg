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
            painter->setPen(QPen(QColor("#fffbf0")));
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

void CPUStack::stackDumpAt(uint_t addr, uint_t csp)
{
    mCsp=csp;
    printDumpAt(addr);
}

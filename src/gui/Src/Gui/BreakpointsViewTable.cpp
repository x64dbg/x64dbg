#include "BreakpointsViewTable.h"
#include "Configuration.h"
#include "Bridge.h"

BreakpointsViewTable::BreakpointsViewTable(QWidget* parent)
    : StdTable(parent),
      mCip(0)
{
    updateColors();
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint, dsint)), this, SLOT(disassembleAtSlot(dsint, dsint)));
}

void BreakpointsViewTable::updateColors()
{
    StdTable::updateColors();
    mCipBackgroundColor = ConfigColor("ThreadCurrentBackgroundColor");
    mCipColor = ConfigColor("ThreadCurrentColor");
}

void BreakpointsViewTable::disassembleAtSlot(dsint cip, dsint addr)
{
    Q_UNUSED(addr)
    mCip = cip;
}

QString BreakpointsViewTable::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);

    if(!col) //address
    {
        QString bpAddrStr = getCellContent(rowBase + rowOffset, col);
        bool valid = false;
#ifdef _WIN64
        duint bpAddr = bpAddrStr.toULongLong(&valid, 16);
#else //x86
        duint bpAddr = bpAddrStr.toULong(&valid, 16);
#endif //_WIN64

        if(valid && bpAddr == mCip)
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(mCipBackgroundColor));
            painter->setPen(QPen(mCipColor));
            painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, bpAddrStr);
            ret = "";
        }
    }

    return ret;
}

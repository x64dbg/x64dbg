#include "BreakpointsViewTable.h"
#include "Configuration.h"

BreakpointsViewTable::BreakpointsViewTable(QWidget* parent)
    : StdTable(parent)
{
    bpAddr = 0;
}

QString BreakpointsViewTable::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    QString bpAddrStr = getCellContent(rowBase + rowOffset, col);

#ifdef _WIN64
    bpAddr = bpAddrStr.toULongLong(0, 16);
#else //x86
    bpAddr = bpAddrStr.toULong(0, 16);
#endif //_WIN64

    if(GetCIP() == bpAddr && !col)
    {
        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("ThreadCurrentBackgroundColor")));
        painter->setPen(QPen(ConfigColor("ThreadCurrentColor"))); //white text
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, bpAddrStr);
        ret = "";
    }

    return ret;
}

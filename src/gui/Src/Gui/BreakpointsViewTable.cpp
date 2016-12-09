#include "BreakpointsViewTable.h"
#include "Configuration.h"

BreakpointsViewTable::BreakpointsViewTable(QWidget* parent)
    : StdTable(parent)
{
    BgColor = QColor(Qt::black); // predefined
    TxtColor = QColor(Qt::white); // predefined
    GetConfigColors();
}

void BreakpointsViewTable::GetConfigColors()
{
    BgColor = ConfigColor("ThreadCurrentBackgroundColor");
    TxtColor = ConfigColor("ThreadCurrentColor");
}

duint BreakpointsViewTable::GetCIP() { return DbgValFromString("cip"); }

QString BreakpointsViewTable::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    duint bpAddr = 0;
    QString ret = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    QString bpAddrStr = getCellContent(rowBase + rowOffset, col);

#ifdef _WIN64
    bpAddr = bpAddrStr.toULongLong(0, 16);
#else //x86
    bpAddr = bpAddrStr.toULong(0, 16);
#endif //_WIN64

    if(GetCIP() == bpAddr && !col)
    {
        painter->fillRect(QRect(x, y, w, h), QBrush(BgColor));
        painter->setPen(QPen(TxtColor));
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, bpAddrStr);
        ret = "";
    }

    return ret;
}

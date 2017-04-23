#include "HandlesWindowViewTable.h"
#include "Configuration.h"
#include "Bridge.h"

HandlesWindowViewTable::HandlesWindowViewTable(QWidget* parent)
    : StdTable(parent)
{
    updateColors();
}

void HandlesWindowViewTable::updateColors()
{
    StdTable::updateColors();
    mBpBackgroundColor = ConfigColor("DisassemblyBreakpointBackgroundColor");
    mBpColor = ConfigColor("DisassemblyBreakpointColor");
}

QString HandlesWindowViewTable::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);

    if(col == 1) // proc address
    {
        QString bpAddrStr = getCellContent(rowBase + rowOffset, col);
        bool valid = false;
#ifdef _WIN64
        duint bpAddr = bpAddrStr.toULongLong(&valid, 16);
#else //x86
        duint bpAddr = bpAddrStr.toULong(&valid, 16);
#endif //_WIN64

        BPXTYPE wBpType = DbgGetBpxTypeAt(bpAddr);
        if(wBpType != bp_none)
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(mBpBackgroundColor));
            painter->setPen(QPen(mBpColor));
            painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, bpAddrStr);
            ret = "";
        }
    }

    return ret;
}

#include "SearchListViewTable.h"
#include "Configuration.h"

SearchListViewTable::SearchListViewTable(StdTable* parent) : StdTable(parent)
{
}

QString SearchListViewTable::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool isaddr=true;
    QString text=StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    if(!DbgIsDebugging())
        isaddr=false;
    if(!getRowCount())
        isaddr=false;
    const char* addrText = text.toUtf8().constData();
    if(!DbgIsValidExpression(addrText))
        isaddr=false;
    uint_t wVA = DbgValFromString(addrText);
    if(!DbgMemIsValidReadPtr(wVA))
        isaddr=false;
    if(col==0 && isaddr)
    {
        BPXTYPE bpxtype=DbgGetBpxTypeAt(wVA);
        bool isbookmark=DbgGetBookmarkAt(wVA);

        if(!isbookmark)
        {
            if(bpxtype&bp_normal) //normal breakpoint
            {
                painter->setPen(QPen(ConfigColor("DisassemblyBreakpointColor")));
                painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //fill red
            }
            else if(bpxtype&bp_hardware) //hardware breakpoint only
            {
                painter->setPen(QPen(ConfigColor("DisassemblyHardwareBreakpointColor")));
                painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyHardwareBreakpointBackgroundColor"))); //fill red
            }
        }
        else //bookmark
        {
            if(bpxtype==bp_none) //bookmark only
            {
                painter->setPen(QPen(ConfigColor("DisassemblyBookmarkColor"))); //black address
                painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBookmarkBackgroundColor"))); //fill bookmark color
            }
            else //bookmark + breakpoint
            {
                QColor color=ConfigColor("DisassemblyBookmarkBackgroundColor");
                if(!color.alpha()) //we don't want transparent text
                    color=textColor;
                painter->setPen(QPen(color));
                if(bpxtype&bp_normal) //bookmark + normal breakpoint
                {
                    painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //fill red
                }
                else if(bpxtype&bp_hardware) //bookmark + hardware breakpoint only
                {
                    painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyHardwareBreakpointBackgroundColor"))); //fill red
                }
                else //other cases (bookmark + memory breakpoint in disassembly) -> do as normal
                {
                    painter->setPen(QPen(ConfigColor("DisassemblyBookmarkColor"))); //black address (DisassemblySelectedAddressColor)
                    painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBookmarkBackgroundColor"))); //fill bookmark color
                }
            }
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, text);
        text="";
    }
    return text;
}

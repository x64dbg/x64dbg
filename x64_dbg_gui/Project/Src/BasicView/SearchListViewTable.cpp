#include "SearchListViewTable.h"
#include "Configuration.h"
#include "RichTextPainter.h"

SearchListViewTable::SearchListViewTable(StdTable* parent) : StdTable(parent)
{
    highlightText = "";
}

QString SearchListViewTable::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool isaddr = true;
    QString text = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    if(!DbgIsDebugging())
        isaddr = false;
    if(!getRowCount())
        isaddr = false;
    const char* addrText = text.toUtf8().constData();
    ULONGLONG val = 0;
    uint_t wVA;
    if(sscanf_s(addrText, "%llX", &val) != 1)
        isaddr = false;
    else
        wVA = val;
    if(col == 0 && isaddr)
    {
        BPXTYPE bpxtype = DbgGetBpxTypeAt(wVA);
        bool isbookmark = DbgGetBookmarkAt(wVA);
        painter->setPen(textColor);
        if(!isbookmark)
        {
            if(bpxtype & bp_normal) //normal breakpoint
            {
                painter->setPen(QPen(ConfigColor("DisassemblyBreakpointColor")));
                painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //fill red
            }
            else if(bpxtype & bp_hardware) //hardware breakpoint only
            {
                painter->setPen(QPen(ConfigColor("DisassemblyHardwareBreakpointColor")));
                painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyHardwareBreakpointBackgroundColor"))); //fill red
            }
        }
        else //bookmark
        {
            if(bpxtype == bp_none) //bookmark only
            {
                painter->setPen(QPen(ConfigColor("DisassemblyBookmarkColor"))); //black address
                painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBookmarkBackgroundColor"))); //fill bookmark color
            }
            else //bookmark + breakpoint
            {
                QColor color = ConfigColor("DisassemblyBookmarkBackgroundColor");
                if(!color.alpha()) //we don't want transparent text
                    color = textColor;
                painter->setPen(QPen(color));
                if(bpxtype & bp_normal) //bookmark + normal breakpoint
                {
                    painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //fill red
                }
                else if(bpxtype & bp_hardware) //bookmark + hardware breakpoint only
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
        text = "";
    }
    else if(highlightText.length() && text.contains(highlightText, Qt::CaseInsensitive))
    {
        //super smart way of splitting while keeping the delimiters (thanks to cypher for guidance)
        int index = -2;
        do
        {
            index = text.indexOf(highlightText, index + 2, Qt::CaseInsensitive);
            if(index != -1)
            {
                text = text.insert(index + highlightText.length(), QChar('\1'));
                text = text.insert(index, QChar('\1'));
            }
        }
        while(index != -1);
        QStringList split = text.split(QChar('\1'), QString::SkipEmptyParts, Qt::CaseInsensitive);

        //create rich text list
        RichTextPainter::CustomRichText_t curRichText;
        curRichText.flags = RichTextPainter::FlagColor;
        curRichText.textColor = textColor;
        curRichText.highlightColor = ConfigColor("SearchListViewHighlightColor");
        QList<RichTextPainter::CustomRichText_t> richText;
        foreach(QString str, split)
        {
            curRichText.text = str;
            curRichText.highlight = !str.compare(highlightText, Qt::CaseInsensitive);
            richText.push_back(curRichText);
        }

        //paint the rich text
        RichTextPainter::paintRichText(painter, x + 1, y, w, h, 4, &richText, getCharWidth());
        text = "";
    }
    return text;
}

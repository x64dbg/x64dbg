#include "SearchListViewTable.h"
#include "Configuration.h"
#include "RichTextPainter.h"
#include "Bridge.h"

SearchListViewTable::SearchListViewTable(StdTable* parent)
    : StdTable(parent),
      bCipBase(false)
{
    highlightText = "";
    updateColors();
}

void SearchListViewTable::updateColors()
{
    StdTable::updateColors();

    mCipBackgroundColor = ConfigColor("DisassemblyCipBackgroundColor");
    mCipColor = ConfigColor("DisassemblyCipColor");
    mBreakpointBackgroundColor = ConfigColor("DisassemblyBreakpointBackgroundColor");
    mBreakpointColor = ConfigColor("DisassemblyBreakpointColor");
    mHardwareBreakpointBackgroundColor = ConfigColor("DisassemblyHardwareBreakpointBackgroundColor");
    mHardwareBreakpointColor = ConfigColor("DisassemblyHardwareBreakpointColor");
    mBookmarkBackgroundColor = ConfigColor("DisassemblyBookmarkBackgroundColor");
    mBookmarkColor = ConfigColor("DisassemblyBookmarkColor");
    mLabelColor = ConfigColor("DisassemblyLabelColor");
    mLabelBackgroundColor = ConfigColor("DisassemblyLabelBackgroundColor");
    mSelectedAddressBackgroundColor = ConfigColor("DisassemblySelectedAddressBackgroundColor");
    mSelectedAddressColor = ConfigColor("DisassemblySelectedAddressColor");
    mAddressBackgroundColor = ConfigColor("DisassemblyAddressBackgroundColor");
    mAddressColor = ConfigColor("DisassemblyAddressColor");
    mTracedBackgroundColor = ConfigColor("DisassemblyTracedBackgroundColor");

    auto a = selectionColor, b = mTracedBackgroundColor;
    mTracedSelectedAddressBackgroundColor = QColor((a.red() + b.red()) / 2, (a.green() + b.green()) / 2, (a.blue() + b.blue()) / 2);
}

QString SearchListViewTable::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool isaddr = true;
    bool wIsSelected = isSelected(rowBase, rowOffset);
    QString text = getCellContent(rowBase + rowOffset, col);
    if(!DbgIsDebugging())
        isaddr = false;
    if(!getRowCount())
        isaddr = false;

    duint wVA = duint(text.toULongLong(&isaddr, 16));
    auto wIsTraced = isaddr && DbgFunctions()->GetTraceRecordHitCount(wVA) != 0;
    QColor lineBackgroundColor;
    bool isBackgroundColorSet;
    if(wIsSelected && wIsTraced)
    {
        lineBackgroundColor = mTracedSelectedAddressBackgroundColor;
        isBackgroundColorSet = true;
    }
    else if(wIsSelected)
    {
        lineBackgroundColor = selectionColor;
        isBackgroundColorSet = true;
    }
    else if(wIsTraced)
    {
        lineBackgroundColor = mTracedBackgroundColor;
        isBackgroundColorSet = true;
    }
    else
    {
        isBackgroundColorSet = false;
    }
    if(isBackgroundColorSet)
        painter->fillRect(QRect(x, y, w, h), QBrush(lineBackgroundColor));

    if(col == 0 && isaddr)
    {
        char label[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(wVA, SEG_DEFAULT, label)) //has label
        {
            char module[MAX_MODULE_SIZE] = "";
            if(DbgGetModuleAt(wVA, module) && !QString(label).startsWith("JMP.&"))
                text += " <" + QString(module) + "." + QString(label) + ">";
            else
                text += " <" + QString(label) + ">";
        }
        BPXTYPE bpxtype = DbgGetBpxTypeAt(wVA);
        bool isbookmark = DbgGetBookmarkAt(wVA);

        duint cip = Bridge::getBridge()->mLastCip;
        if(bCipBase)
        {
            duint base = DbgFunctions()->ModBaseFromAddr(cip);
            if(base)
                cip = base;
        }

        if(DbgIsDebugging() && wVA == cip) //cip + not running
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(mCipBackgroundColor));
            if(!isbookmark) //no bookmark
            {
                if(bpxtype & bp_normal) //normal breakpoint
                {
                    QColor & bpColor = mBreakpointBackgroundColor;
                    if(!bpColor.alpha()) //we don't want transparent text
                        bpColor = mBreakpointColor;
                    if(bpColor == mCipBackgroundColor)
                        bpColor = mCipColor;
                    painter->setPen(bpColor);
                }
                else if(bpxtype & bp_hardware) //hardware breakpoint only
                {
                    QColor hwbpColor = mHardwareBreakpointBackgroundColor;
                    if(!hwbpColor.alpha()) //we don't want transparent text
                        hwbpColor = mHardwareBreakpointColor;
                    if(hwbpColor == mCipBackgroundColor)
                        hwbpColor = mCipColor;
                    painter->setPen(hwbpColor);
                }
                else //no breakpoint
                {
                    painter->setPen(mCipColor);
                }
            }
            else //bookmark
            {
                QColor bookmarkColor = mBookmarkBackgroundColor;
                if(!bookmarkColor.alpha()) //we don't want transparent text
                    bookmarkColor = mBookmarkColor;
                if(bookmarkColor == mCipBackgroundColor)
                    bookmarkColor = mCipColor;
                painter->setPen(bookmarkColor);
            }
        }
        else //non-cip address
        {
            if(!isbookmark) //no bookmark
            {
                if(*label) //label
                {
                    if(bpxtype == bp_none) //label only : fill label background
                    {
                        painter->setPen(mLabelColor); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint
                    {
                        if(bpxtype & bp_normal) //label + normal breakpoint
                        {
                            painter->setPen(mBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + hardware breakpoint only
                        {
                            painter->setPen(mHardwareBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                        else //other cases -> do as normal
                        {
                            painter->setPen(mLabelColor); //red -> address + label text
                            painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                        }
                    }
                }
                else //no label
                {
                    if(bpxtype == bp_none) //no label, no breakpoint
                    {
                        QColor background;
                        if(wIsSelected)
                        {
                            background = mSelectedAddressBackgroundColor;
                            painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                        }
                        else
                        {
                            background = mAddressBackgroundColor;
                            painter->setPen(mAddressColor); //DisassemblyAddressColor
                        }
                        if(background.alpha())
                            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
                    }
                    else //breakpoint only
                    {
                        if(bpxtype & bp_normal) //normal breakpoint
                        {
                            painter->setPen(mBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //hardware breakpoint only
                        {
                            painter->setPen(mHardwareBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (memory breakpoint in disassembly) -> do as normal
                        {
                            QColor background;
                            if(wIsSelected)
                            {
                                background = mSelectedAddressBackgroundColor;
                                painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                            }
                            else
                            {
                                background = mAddressBackgroundColor;
                                painter->setPen(mAddressColor);
                            }
                            if(background.alpha())
                                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
                        }
                    }
                }
            }
            else //bookmark
            {
                if(*label) //label + bookmark
                {
                    if(bpxtype == bp_none) //label + bookmark
                    {
                        painter->setPen(mLabelColor); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint + bookmark
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        painter->setPen(color);
                        if(bpxtype & bp_normal) //label + bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                    }
                }
                else //bookmark, no label
                {
                    if(bpxtype == bp_none) //bookmark only
                    {
                        painter->setPen(mBookmarkColor); //black address
                        painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                    }
                    else //bookmark + breakpoint
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        painter->setPen(color);
                        if(bpxtype & bp_normal) //bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (bookmark + memory breakpoint in disassembly) -> do as normal
                        {
                            painter->setPen(mBookmarkColor); //black address
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                        }
                    }
                }
            }
        }
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, text);
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
        RichTextPainter::List richText;
        foreach(QString str, split)
        {
            curRichText.text = str;
            curRichText.highlight = !str.compare(highlightText, Qt::CaseInsensitive);
            richText.push_back(curRichText);
        }

        //paint the rich text
        RichTextPainter::paintRichText(painter, x + 1, y, w, h, 4, richText, mFontMetrics);
        text = "";
    }
    return text;
}

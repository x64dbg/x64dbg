#include "ScriptView.h"

ScriptView::ScriptView(StdTable *parent) : StdTable(parent)
{
    enableMultiSelection(false);

    int charwidth=QFontMetrics(this->font()).width(QChar(' '));

    addColumnAt(8+charwidth*4, "Line", false);
    addColumnAt(8+charwidth*60, "Text", false);
    addColumnAt(8+charwidth*40, "Info", false);

    const char* sample_script[6]={"var test,123", "mov test,$pid", "mov eax,pid", "estep", "mov test,eax", "ret"};

    setRowCount(6);

    for(int i=0; i<6; i++)
    {
        setCellContent(i, 1, sample_script[i]);
    }
}

QString ScriptView::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool wIsSelected=isSelected(rowBase, rowOffset);
    // Highlight if selected
    if(wIsSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#C0C0C0")));
    QString returnString;
    switch(col)
    {
    case 0: //line number
    {
        int line=rowOffset+1;
        returnString=returnString.sprintf("%.4d", line);
        painter->save();
        if(line==2)
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#000000")));
            painter->setPen(QPen(QColor("#FFFFFF"))); //black address
        }
        else if(line==5)
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#ff0000")));
            painter->setPen(QPen(QColor("#000000"))); //black address
        }
        else
        {
            if(wIsSelected)
                painter->setPen(QPen(QColor("#000000"))); //black address
            else
                painter->setPen(QPen(QColor("#808080")));
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, returnString);
        painter->restore();
        returnString="";
    }
    break;

    case 1: //command
    {
        returnString=getCellContent(rowOffset, col);
    }
    break;

    case 2: //info
    {
        returnString=getCellContent(rowOffset, col);
    }
    break;
    }
    return returnString;
}

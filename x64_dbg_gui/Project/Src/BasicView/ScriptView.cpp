#include "ScriptView.h"

ScriptView::ScriptView(StdTable *parent) : StdTable(parent)
{
    enableMultiSelection(false);

    int charwidth=QFontMetrics(this->font()).width(QChar(' '));

    addColumnAt(8+charwidth*4, "Line", false);
    addColumnAt(8+charwidth*60, "Text", false);
    addColumnAt(8+charwidth*40, "Info", false);

    setIp(0); //no IP

    connect(Bridge::getBridge(), SIGNAL(scriptAddLine(QString)), this, SLOT(addLine(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptClear()), this, SLOT(clear()));
    connect(Bridge::getBridge(), SIGNAL(scriptSetIp(int)), this, SLOT(setIp(int)));
    connect(Bridge::getBridge(), SIGNAL(scriptError(int,QString)), this, SLOT(error(int,QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetTitle(QString)), this, SLOT(setTitle(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetInfoLine(int,QString)), this, SLOT(setInfoLine(int,QString)));
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
        int line=rowBase+rowOffset+1;
        returnString=returnString.sprintf("%.4d", line);
        painter->save();
        if(line==mIpLine) //IP
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#000000")));
            if(DbgScriptBpGet(line)) //breakpoint
                painter->setPen(QPen(QColor("#FF0000"))); //red address
            else
                painter->setPen(QPen(QColor("#FFFFFF"))); //black address
        }
        else if(DbgScriptBpGet(line)) //breakpoint
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
        returnString=getCellContent(rowBase+rowOffset, col);
    }
    break;

    case 2: //info
    {
        returnString=getCellContent(rowBase+rowOffset, col);
    }
    break;
    }
    return returnString;
}

void ScriptView::addLine(QString text)
{
    int rows=getRowCount();
    setRowCount(rows+1);
    setCellContent(rows, 1, text);
    reloadData(); //repaint
}

void ScriptView::clear()
{
    setRowCount(0);
    mIpLine=0;
    reloadData(); //repaint
}

void ScriptView::setIp(int line)
{
    if(!isValidIndex(line-1, 0))
        mIpLine=0;
    else
        mIpLine=line;
    reloadData(); //repaint
}

void ScriptView::error(int line, QString message)
{
    QString title;
    if(isValidIndex(line-1, 0))
        title=title.sprintf("Error on line %.4d!", line);
    else
        title="Script Error!";
    QMessageBox msg(QMessageBox::Critical, title, message);
    msg.setWindowIcon(QIcon(":/icons/images/script-error.png"));
    msg.exec();
}

void ScriptView::setTitle(QString title)
{
    setWindowTitle(title);
}

void ScriptView::setInfoLine(int line, QString info)
{
    setCellContent(line-1, 2, info);
    reloadData(); //repaint
}

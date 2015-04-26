#include "SourceView.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include "Configuration.h"

SourceView::SourceView(QString path, int line, StdTable* parent) : StdTable(parent)
{
    mSourcePath = path;

    addColumnAt(8 + 4 * getCharWidth(), "Line", true);
    addColumnAt(0, "Code", true);

    loadFile();
    setInstructionPointer(line);

    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    setupContextMenu();
}

void SourceView::contextMenuSlot(const QPoint & pos)
{
    QMenu* wMenu = new QMenu(this);

    int line = getInitialSelection() + 1;
    int_t addr = DbgFunctions()->GetAddrFromLine(mSourcePath.toUtf8().constData(), line);
    if(addr)
        wMenu->addAction(mFollowInDisasm);

    wMenu->exec(mapToGlobal(pos));
}

void SourceView::setupContextMenu()
{
    mFollowInDisasm = new QAction("Follow in &Disassembler", this);
    connect(mFollowInDisasm, SIGNAL(triggered()), this, SLOT(followInDisasmSlot()));
}

void SourceView::setSelection(int line)
{
    int offset = line - 1;
    if(isValidIndex(offset, 0))
        setSingleSelection(offset);
    reloadData(); //repaint
}

void SourceView::setInstructionPointer(int line)
{
    int offset = line - 1;
    if(!isValidIndex(offset, 0))
    {
        mIpLine = 0;
        return;
    }
    mIpLine = line;
    int rangefrom = getTableOffset();
    int rangeto = rangefrom + getViewableRowsCount() - 1;
    if(offset < rangefrom) //ip lays before the current view
        setTableOffset(offset);
    else if(offset > (rangeto - 1)) //ip lays after the current view
        setTableOffset(offset - getViewableRowsCount() + 2);
    setSingleSelection(offset);
    reloadData(); //repaint
}

QString SourceView::getSourcePath()
{
    return mSourcePath;
}

void SourceView::loadFile()
{
    QFile file(mSourcePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        return; //error?
    }
    QTextStream in(&file);
    int lineNum = 0;
    while(!in.atEnd())
    {
        QString line = in.readLine().replace('\t', "    "); //replace tabs with four spaces
        setRowCount(lineNum + 1);
        setCellContent(lineNum, 0, QString().sprintf("%04d", lineNum + 1));
        setCellContent(lineNum, 1, line);
        lineNum++;
    }
    reloadData();
    file.close();
}

QString SourceView::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    painter->save();
    bool wIsSelected = isSelected(rowBase, rowOffset);
    // Highlight if selected
    if(wIsSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor)); //ScriptViewSelectionColor
    QString returnString;
    int line = rowBase + rowOffset + 1;
    switch(col)
    {
    case 0: //line number
    {
        if(line == mIpLine) //IP
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyCipBackgroundColor")));
            painter->setPen(QPen(ConfigColor("DisassemblyCipColor"))); //white address (ScriptViewIpTextColor)
        }
        else
            painter->setPen(QPen(this->textColor));
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, QString().sprintf("%04d", line));
    }
    break;

    case 1: //command
    {
        returnString = getCellContent(rowBase + rowOffset, col); //TODO: simple keyword/regex-based syntax highlighting
    }
    break;
    }
    painter->restore();
    return returnString;
}

void SourceView::followInDisasmSlot()
{
    int line = getInitialSelection() + 1;
    int_t addr = DbgFunctions()->GetAddrFromLine(mSourcePath.toUtf8().constData(), line);
    DbgCmdExecDirect(QString().sprintf("disasm %p", addr).toUtf8().constData());
    emit showCpu();
}

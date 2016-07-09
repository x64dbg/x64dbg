#include "SourceView.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include "Configuration.h"

SourceView::SourceView(QString path, int line, StdTable* parent)
    : ReferenceView(true, parent)
{
    mSourcePath = path;
    mList->enableColumnSorting(false);
    mSearchList->enableColumnSorting(false);

    addColumnAt(sizeof(duint) * 2, "Address");
    addColumnAt(6, "Line");
    addColumnAt(0, "Code");

    loadFile();
    setSelection(line);
}

void SourceView::setSelection(int line)
{
    int offset = line - 1;
    if(mCurList->isValidIndex(offset, 0))
    {
        int rangefrom = mCurList->getTableOffset();
        int rangeto = rangefrom + mCurList->getViewableRowsCount() - 1;
        if(offset < rangefrom) //ip lays before the current view
            mCurList->setTableOffset(offset);
        else if(offset > (rangeto - 1)) //ip lays after the current view
            mCurList->setTableOffset(offset - mCurList->getViewableRowsCount() + 2);
        mCurList->setSingleSelection(offset);
    }
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
        duint addr = DbgFunctions()->GetAddrFromLine(mSourcePath.toUtf8().constData(), lineNum + 1);
        if(addr)
            setCellContent(lineNum, 0, ToPtrString(addr));
        setCellContent(lineNum, 1, QString("%1").arg(lineNum + 1));
        setCellContent(lineNum, 2, line);
        lineNum++;
    }
    reloadData();
    file.close();
}

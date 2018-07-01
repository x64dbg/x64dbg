#include "SourceView.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QDesktopServices>
#include "Configuration.h"

SourceView::SourceView(QString path, duint addr, QWidget* parent)
    : ReferenceView(true, parent),
      mSourcePath(path),
      mIpLine(0),
      mModBase(DbgFunctions()->ModBaseFromAddr(addr))
{
    stdList()->enableColumnSorting(false);
    stdSearchList()->enableColumnSorting(false);

    addColumnAtRef(sizeof(duint) * 2, tr("Address"));
    addColumnAtRef(6, tr("Line"));
    addColumnAtRef(0, tr("Code"));

    connect(this, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(sourceContextMenu(QMenu*)));

    loadFile();
    setSelection(addr);

    mMenuBuilder = new MenuBuilder(this);
    mMenuBuilder->addAction(makeAction(DIcon("source.png"), tr("Open source file"), SLOT(openSourceFileSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("source_show_in_folder.png"), tr("Show source file in directory"), SLOT(showInDirectorySlot())));
    mMenuBuilder->loadFromConfig();
}

void SourceView::setSelection(duint addr)
{
    int line = 0;
    if(!DbgFunctions()->GetSourceFromAddr(addr, nullptr, &line))
        return;
    mCurList->scrollSelect(line - 1);
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
        duint addr = DbgFunctions()->GetAddrFromLineEx(mModBase, mSourcePath.toUtf8().constData(), lineNum + 1);
        if(addr)
            setCellContent(lineNum, 0, ToPtrString(addr));
        setCellContent(lineNum, 1, QString("%1").arg(lineNum + 1));
        setCellContent(lineNum, 2, line);
        lineNum++;
    }
    reloadData();
    file.close();
}

void SourceView::sourceContextMenu(QMenu* menu)
{
    menu->addSeparator();
    mMenuBuilder->build(menu);
}

void SourceView::openSourceFileSlot()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(mSourcePath));
}

void SourceView::showInDirectorySlot()
{
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(mSourcePath);
    auto process = new QProcess(this);
    process->start("explorer.exe", args);
}

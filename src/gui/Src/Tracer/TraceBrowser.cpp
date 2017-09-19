#include "TraceBrowser.h"
#include "TraceFileReader.h"
#include "BrowseDialog.h"

TraceBrowser::TraceBrowser(QWidget* parent) : AbstractTableView(parent)
{
    mTraceFile = nullptr;
    addColumnAt(getCharWidth() * 2 * 8 + 8, "", false); //index
    addColumnAt(getCharWidth() * 2 * sizeof(dsint) + 8, "", false); //address
    addColumnAt(getCharWidth() * 2 * 12 + 8, "", false); //bytes
    addColumnAt(getCharWidth() * 40, "", false); //disassembly
    addColumnAt(1000, "", false); //comments

    setShowHeader(false); //hide header

    setupRightClickContextMenu();

    Initialize();
}


QString TraceBrowser::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    switch(col)
    {
    case 0: //index
        return QString::number(rowOffset + rowBase);

    case 1: //address
    default:
        return QString("deubg");
    }
}

void TraceBrowser::prepareData()
{
    auto viewables = getViewableRowsCount();
    int lines = 10;
    if(mTraceFile != nullptr)
    {
        if(mTraceFile->Progress() == 100)
        {
            if(mTraceFile->Length() < getTableOffset() + viewables)
                lines = mTraceFile->Length() - getTableOffset();
            else
                lines = viewables;
        }
    }
    setNbrOfLineToPrint(lines);
}

void TraceBrowser::setupRightClickContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);
    mMenuBuilder->addAction(makeAction(DIcon("open.png"), tr("Open"), SLOT(openFileSlot())), [this](QMenu*)
    {
        return mTraceFile == nullptr;
    });
}

void TraceBrowser::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    mMenuBuilder->build(&menu);
    menu.exec(event->globalPos());
}

void TraceBrowser::openFileSlot()
{
    BrowseDialog browse(this, tr("Open run trace file"), tr("Open trace file"), tr("Run trace files (*.%1);;All files (*.*)").arg(ArchValue("trace32", "trace64")), QApplication::applicationDirPath(), false);
    if(browse.exec() != QDialog::Accepted)
        return;
    mTraceFile = new TraceFileReader(this);
    connect(mTraceFile, SIGNAL(parseFinished()), this, SLOT(parseFinishedSlot()));
    mTraceFile->Open(browse.path);
}

void TraceBrowser::parseFinishedSlot()
{
    if(mTraceFile->isError())
    {
        SimpleErrorBox(this, tr("Error"), "Error when opening run trace file");
        delete mTraceFile;
        mTraceFile = nullptr;
    }
    reloadData();
}

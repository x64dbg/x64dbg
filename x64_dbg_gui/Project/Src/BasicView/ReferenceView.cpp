#include "ReferenceView.h"

ReferenceView::ReferenceView()
{
    // Setup SearchListView settings
    mSearchStartCol = 1;

    // Create search progress bar
    mSearchProgress = new QProgressBar();
    mSearchProgress->setRange(0, 100);
    mSearchProgress->setTextVisible(false);
    mSearchProgress->setMaximumHeight(15);

    // Add the progress bar to the main layout
    mMainLayout->addWidget(mSearchProgress);

    // Setup signals
    connect(Bridge::getBridge(), SIGNAL(referenceAddColumnAt(int,QString)), this, SLOT(addColumnAt(int,QString)));
    connect(Bridge::getBridge(), SIGNAL(referenceSetRowCount(int_t)), this, SLOT(setRowCount(int_t)));
    connect(Bridge::getBridge(), SIGNAL(referenceDeleteAllColumns()), this, SLOT(deleteAllColumns()));
    connect(Bridge::getBridge(), SIGNAL(referenceSetCellContent(int,int,QString)), this, SLOT(setCellContent(int,int,QString)));
    connect(Bridge::getBridge(), SIGNAL(referenceReloadData()), this, SLOT(reloadData()));
    connect(Bridge::getBridge(), SIGNAL(referenceSetSingleSelection(int,bool)), this, SLOT(setSingleSelection(int,bool)));
    connect(Bridge::getBridge(), SIGNAL(referenceSetProgress(int)), mSearchProgress, SLOT(setValue(int)));
    connect(this, SIGNAL(listContextMenuSignal(QPoint)), this, SLOT(referenceContextMenu(QPoint)));

    setupContextMenu();
}

void ReferenceView::setupContextMenu()
{
    mFollowAddress = new QAction("&Follow in Disassembler", this);
    connect(mFollowAddress, SIGNAL(triggered()), this, SLOT(followAddress()));

    mFollowDumpAddress = new QAction("Follow in &Dump", this);
    connect(mFollowDumpAddress, SIGNAL(triggered()), this, SLOT(followDumpAddress()));
}

void ReferenceView::addColumnAt(int width, QString title)
{
    QFont wFont("Monospace", 8);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);
    int charwidth=QFontMetrics(wFont).width(QChar(' '));
    if(width)
        width=charwidth*width+8;
    else
        width=0;
    mSearchBox->setText("");
    mList->addColumnAt(width, title, true);
    mSearchList->addColumnAt(width, title, true);
}

void ReferenceView::setRowCount(int_t count)
{
    mSearchBox->setText("");
    mList->setRowCount(count);
}

void ReferenceView::deleteAllColumns()
{
    mList->deleteAllColumns();
}

void ReferenceView::setCellContent(int r, int c, QString s)
{
    mSearchBox->setText("");
    mList->setCellContent(r, c, s);
}

void ReferenceView::reloadData()
{
    mSearchBox->setText("");
    mList->reloadData();
}

void ReferenceView::setSingleSelection(int index, bool scroll)
{
    mSearchBox->setText("");
    mList->setSingleSelection(index);
    if(scroll) //TODO: better scrolling
        mList->setTableOffset(index);
}

void ReferenceView::referenceContextMenu(const QPoint &pos)
{
    if(!this->mCurList->getRowCount())
        return;
    const char* addrText = this->mCurList->getCellContent(this->mCurList->getInitialSelection(), 0).toUtf8().constData();
    if(!DbgIsValidExpression(addrText))
        return;
    uint_t addr = DbgValFromString(addrText);
    if(!DbgMemIsValidReadPtr(addr))
        return;
    QMenu* wMenu = new QMenu(this);
    wMenu->addAction(mFollowAddress);
    wMenu->addAction(mFollowDumpAddress);
    wMenu->exec(pos);
}

void ReferenceView::followAddress()
{
    DbgCmdExecDirect(QString("disasm " + this->mCurList->getCellContent(this->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

void ReferenceView::followDumpAddress()
{
    DbgCmdExecDirect(QString("dump " + this->mCurList->getCellContent(this->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

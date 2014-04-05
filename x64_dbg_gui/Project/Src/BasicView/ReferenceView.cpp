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
    connect(Bridge::getBridge(), SIGNAL(referenceSetSearchStartCol(int)), this, SLOT(setSearchStartCol(int)));
    connect(this, SIGNAL(listContextMenuSignal(QPoint)), this, SLOT(referenceContextMenu(QPoint)));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followAddress()));

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
    mList->setTableOffset(0);
    mList->setSingleSelection(0);
    mList->deleteAllColumns();
    mList->reloadData();
    mSearchStartCol = 1;
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

void ReferenceView::setSearchStartCol(int col)
{
    if(col < mList->getColumnCount())
        this->mSearchStartCol = col;
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
    wMenu->addSeparator();

    //add copy actions
    int count=this->mCurList->getColumnCount();
    for(int i=0; i<count; i++)
    {
        wMenu->addAction(new QAction(QString("Copy " + this->mCurList->getColTitle(i)), this));
        wMenu->actions().last()->setObjectName(QString("COPY|")+QString().sprintf("%d", i));
        connect(wMenu->actions().last(), SIGNAL(triggered()), this, SLOT(copySlot()));
    }

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

void ReferenceView::copySlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action && action->objectName().startsWith("COPY|"))
    {
        bool ok=false;
        int row=action->objectName().mid(5).toInt(&ok);
        if(ok)
        {
            Bridge::CopyToClipboard(this->mCurList->getCellContent(this->mCurList->getInitialSelection(), row).toUtf8().constData());
        }
    }
}

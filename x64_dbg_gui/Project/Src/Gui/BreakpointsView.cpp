#include "BreakpointsView.h"

BreakpointsView::BreakpointsView(QWidget *parent) :QWidget(parent)
{
    mHardBPTable = new StdTable(this);
    mHardBPTable->setMaximumHeight(100);
    mSoftBPTable = new StdTable(this);

    mHardBPTable->addColumnAt(50, false);
    mHardBPTable->addColumnAt(50, false);

    mSoftBPTable->addColumnAt(50, false);
    mSoftBPTable->addColumnAt(50, false);

    mVertLayout = new QVBoxLayout;
    mVertLayout->setSpacing(0);
    mVertLayout->setContentsMargins(0, 0, 0, 0);

    mVertLayout->addWidget(mHardBPTable);
    mVertLayout->addWidget(mSoftBPTable);

    this->setLayout(mVertLayout);
}


void BreakpointsView::paintEvent(QPaintEvent* event)
{
    BPMAP wBPList;
    int wI;

    // Hardware
    DbgGetBpList(bp_hardware, &wBPList);
    mHardBPTable->setRowCount(wBPList.count);

    for(wI = 0; wI < wBPList.count; wI++)
    {
        mHardBPTable->setCellContent(wI, 0, QString("%1").arg(wBPList.bp[wI].addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
    }

    // Software
    DbgGetBpList(bp_normal, &wBPList);
    mSoftBPTable->setRowCount(wBPList.count);

    for(wI = 0; wI < wBPList.count; wI++)
    {
        mSoftBPTable->setCellContent(wI, 0, QString("%1").arg(wBPList.bp[wI].addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
    }


    QWidget::paintEvent(event);
}

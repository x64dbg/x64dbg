#include "CPUWidget.h"
#include "ui_CPUWidget.h"

CPUWidget::CPUWidget(QWidget *parent) :QWidget(parent), ui(new Ui::CPUWidget)
{
    ui->setupUi(this);
    setDefaultDisposition();

    mDisas = new CPUDisassembly(0);
    mSideBar = new CPUSideBar(mDisas);
    connect(mDisas,SIGNAL(tableOffsetChanged(int_t)),mSideBar,SLOT(changeTopmostAddress(int_t)));
    connect(mDisas,SIGNAL(viewableRows(int)),mSideBar,SLOT(setViewableRows(int)));
    connect(mDisas,SIGNAL(repainted()),mSideBar,SLOT(repaint()));
    connect(mDisas,SIGNAL(selectionChanged(int_t)),mSideBar,SLOT(setSelection(int_t)));
    connect(Bridge::getBridge(),SIGNAL(dbgStateChanged(DBGSTATE)),mSideBar,SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(),SIGNAL(updateSideBar()),mSideBar,SLOT(repaint()));

    QSplitter* splitter = new QSplitter(this);
    splitter->addWidget(mSideBar);
    splitter->addWidget(mDisas);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(1);

    ui->mTopLeftUpperFrameLayout->addWidget(splitter);

    mInfo = new CPUInfoBox();
    ui->mTopLeftLowerFrameLayout->addWidget(mInfo);
    int height = mInfo->getHeight();
    ui->mTopLeftLowerFrame->setMinimumHeight(height+2);
    ui->mTopLeftLowerFrame->setMaximumHeight(height+2);

    connect(mDisas, SIGNAL(selectionChanged(int_t)), mInfo, SLOT(disasmSelectionChanged(int_t)));

    mGeneralRegs = new RegistersView(0);

    //TODO: add more tabs
    mRegsTab = new QTabWidget(this);
    mRegsTab->addTab(mGeneralRegs, "General");

    ui->mTopRightFrameLayout->addWidget(mRegsTab);

    CPUDump* hx = new CPUDump(0); //dump widget
    ui->mBotLeftFrameLayout->addWidget(hx);

    CPUStack* st = new CPUStack(0); //stack widget
    ui->mBotRightFrameLayout->addWidget(st);
}

CPUWidget::~CPUWidget()
{
    delete ui;
}

void CPUWidget::setDefaultDisposition(void)
{
    QList<int> sizesList;
    int wTotalSize;

    // Vertical Splitter
    wTotalSize = ui->mVSplitter->widget(0)->size().height() + ui->mVSplitter->widget(1)->size().height();

    sizesList.append(wTotalSize*70/100);
    sizesList.append(wTotalSize-wTotalSize*70/100);

    ui->mVSplitter->setSizes(sizesList);

    // Top Horizontal Splitter
    wTotalSize = ui->mTopHSplitter->widget(0)->size().height() + ui->mTopHSplitter->widget(1)->size().height();

    sizesList.append(wTotalSize*70/100);
    sizesList.append(wTotalSize-wTotalSize*70/100);

    ui->mTopHSplitter->setSizes(sizesList);

    // Bottom Horizontal Splitter
    wTotalSize = ui->mBotHSplitter->widget(0)->size().height() + ui->mBotHSplitter->widget(1)->size().height();

    sizesList.append(wTotalSize*70/100);
    sizesList.append(wTotalSize-wTotalSize*70/100);

    ui->mBotHSplitter->setSizes(sizesList);
}


QVBoxLayout* CPUWidget::getTopLeftUpperWidget(void)
{
    return ui->mTopLeftUpperFrameLayout;
}

QVBoxLayout* CPUWidget::getTopLeftLowerWidget(void)
{
    return ui->mTopLeftLowerFrameLayout;
}


QVBoxLayout* CPUWidget::getTopRightWidget(void)
{
    return ui->mTopRightFrameLayout;
}


QVBoxLayout* CPUWidget::getBotLeftWidget(void)
{
    return ui->mBotLeftFrameLayout;
}


QVBoxLayout* CPUWidget::getBotRightWidget(void)
{
    return ui->mBotRightFrameLayout;
}

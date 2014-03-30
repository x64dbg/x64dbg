#include "CPUWidget.h"
#include "ui_CPUWidget.h"

CPUWidget::CPUWidget(QWidget *parent) :QWidget(parent), ui(new Ui::CPUWidget)
{
    ui->setupUi(this);
    setDefaultDisposition();

    mDisas = new CPUDisassembly(0);
    ui->mTopLeftUpperFrameLayout->addWidget(mDisas);

    mInfo = new InfoBox();
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

    //cw = new ColumnWidget(3, this);
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

void CPUWidget::runSelection()
{
    if(!DbgIsDebugging())
        return;
    QString command = "bp " + QString("%1").arg(mDisas->rvaToVa(mDisas->getInitialSelection()), sizeof(int_t)*2, 16, QChar('0')).toUpper() + ", ss";
    if(DbgCmdExecDirect(command.toUtf8().constData()))
        DbgCmdExecDirect("run");
}

#include "CPUWidget.h"
#include "ui_CPUWidget.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include "CPUSideBar.h"
#include "CPUDisassembly.h"
#include "CPUMultiDump.h"
#include "CPUStack.h"
#include "RegistersView.h"
#include "CPUInfoBox.h"
#include "CPUArgumentWidget.h"
#include "Configuration.h"

CPUWidget::CPUWidget(QWidget* parent) : QWidget(parent), ui(new Ui::CPUWidget)
{
    ui->setupUi(this);
    setDefaultDisposition();

    setStyleSheet("AbstractTableView:focus, RegistersView:focus, CPUSideBar:focus { border: 1px solid #000000; }");

    mDisas = new CPUDisassembly(this);
    mSideBar = new CPUSideBar(mDisas);
    mArgumentWidget = new CPUArgumentWidget(this);
    connect(mDisas, SIGNAL(tableOffsetChanged(dsint)), mSideBar, SLOT(changeTopmostAddress(dsint)));
    connect(mDisas, SIGNAL(viewableRows(int)), mSideBar, SLOT(setViewableRows(int)));
    connect(mDisas, SIGNAL(selectionChanged(dsint)), mSideBar, SLOT(setSelection(dsint)));
    connect(mDisas, SIGNAL(disassembledAt(dsint, dsint, bool, dsint)), mArgumentWidget, SLOT(disassembledAtSlot(dsint, dsint, bool, dsint)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), mSideBar, SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(updateSideBar()), mSideBar, SLOT(reload()));
    connect(Bridge::getBridge(), SIGNAL(updateArgumentView()), mArgumentWidget, SLOT(refreshData()));
    mDisas->setCodeFoldingManager(mSideBar->getCodeFoldingManager());

    QSplitter* splitter = new QSplitter(this);
    splitter->addWidget(mSideBar);
    splitter->addWidget(mDisas);
    splitter->setChildrenCollapsible(false);
    splitter->setCollapsible(0, true); //allow collapsing of the SideBar
    splitter->setHandleWidth(1);

    ui->mTopLeftVSplitter->setSizes(QList<int>() << 100 << 1);
    ui->mTopLeftVSplitter->setCollapsible(1, true); //allow collapsing of the InfoBox

    ui->mTopLeftUpperFrameLayout->addWidget(splitter);

    mInfo = new CPUInfoBox();
    ui->mTopLeftLowerFrameLayout->addWidget(mInfo);
    int height = mInfo->getHeight();
    ui->mTopLeftLowerFrame->setMinimumHeight(height + 2);

    connect(mDisas, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));

    mDump = new CPUMultiDump(mDisas, 5, 0); //dump widget
    ui->mBotLeftFrameLayout->addWidget(mDump);

    mGeneralRegs = new RegistersView(this, mDump);
    mGeneralRegs->setFixedWidth(1000);
    mGeneralRegs->ShowFPU(true);

    QScrollArea* upperScrollArea = new QScrollArea(this);
    upperScrollArea->setFrameShape(QFrame::NoFrame);
    upperScrollArea->setWidget(mGeneralRegs);

    upperScrollArea->horizontalScrollBar()->setStyleSheet(ConfigHScrollBarStyle());
    upperScrollArea->verticalScrollBar()->setStyleSheet(ConfigVScrollBarStyle());

    QPushButton* button_changeview = new QPushButton("", this);
    button_changeview->setStyleSheet("Text-align:left;padding: 4px;padding-left: 10px;");
    connect(button_changeview, SIGNAL(clicked()), mGeneralRegs, SLOT(onChangeFPUViewAction()));
    mGeneralRegs->SetChangeButton(button_changeview);

    ui->mTopRightVSplitter->setSizes(QList<int>() << 87 << 14);
    ui->mTopRightVSplitter->setCollapsible(1, true); //allow collapsing of the ArgumentWidget

    ui->mTopRightUpperFrameLayout->addWidget(button_changeview);
    ui->mTopRightUpperFrameLayout->addWidget(upperScrollArea);

    ui->mTopRightLowerFrameLayout->addWidget(mArgumentWidget);

    mStack = new CPUStack(mDump, 0); //stack widget
    ui->mBotRightFrameLayout->addWidget(mStack);

    // load column config
    mDisas->loadColumnFromConfig("CPUDisassembly");
    mStack->loadColumnFromConfig("CPUStack");
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

    sizesList.append(wTotalSize * 70 / 100);
    sizesList.append(wTotalSize - wTotalSize * 70 / 100);

    ui->mVSplitter->setSizes(sizesList);

    // Top Horizontal Splitter
    wTotalSize = ui->mTopHSplitter->widget(0)->size().height() + ui->mTopHSplitter->widget(1)->size().height();

    sizesList.append(wTotalSize * 70 / 100);
    sizesList.append(wTotalSize - wTotalSize * 70 / 100);

    ui->mTopHSplitter->setSizes(sizesList);

    // Bottom Horizontal Splitter
    wTotalSize = ui->mBotHSplitter->widget(0)->size().height() + ui->mBotHSplitter->widget(1)->size().height();

    sizesList.append(wTotalSize * 70 / 100);
    sizesList.append(wTotalSize - wTotalSize * 70 / 100);

    ui->mBotHSplitter->setSizes(sizesList);
}

void CPUWidget::setDisasmFocus()
{
    mDisas->setFocus();
}

QVBoxLayout* CPUWidget::getTopLeftUpperWidget()
{
    return ui->mTopLeftUpperFrameLayout;
}

QVBoxLayout* CPUWidget::getTopLeftLowerWidget()
{
    return ui->mTopLeftLowerFrameLayout;
}

QVBoxLayout* CPUWidget::getTopRightWidget()
{
    return ui->mTopRightUpperFrameLayout;
}

QVBoxLayout* CPUWidget::getBotLeftWidget()
{
    return ui->mBotLeftFrameLayout;
}

QVBoxLayout* CPUWidget::getBotRightWidget()
{
    return ui->mBotRightFrameLayout;
}

CPUSideBar* CPUWidget::getSidebarWidget()
{
    return mSideBar;
}

CPUDisassembly* CPUWidget::getDisasmWidget()
{
    return mDisas;
}

CPUMultiDump* CPUWidget::getDumpWidget()
{
    return mDump;
}

CPUStack* CPUWidget::getStackWidget()
{
    return mStack;
}

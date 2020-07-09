#include "TraceWidget.h"
#include "ui_TraceWidget.h"
#include "TraceBrowser.h"
#include "RegistersView.h"
#include "CPUInfoBox.h"

TraceWidget::TraceWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::TraceWidget)
{
    ui->setupUi(this);

    mTraceWidget = new TraceBrowser(this);
    mOverview = new StdTable(this);
    mInfo = new CPUInfoBox(this);
    mGeneralRegs = new RegistersView(this);
    //disasm
    ui->mTopLeftUpperRightFrameLayout->addWidget(mTraceWidget);
    //registers
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
    connect(mTraceWidget, SIGNAL(updateTraceRegistersView(void*)), this, SLOT(updateTraceRegistersView(void*)));

    mGeneralRegs->SetChangeButton(button_changeview);

    ui->mTopRightUpperFrameLayout->addWidget(button_changeview);
    ui->mTopRightUpperFrameLayout->addWidget(upperScrollArea);
    //info
    ui->mTopLeftLowerFrameLayout->addWidget(mInfo);
    int height = mInfo->getHeight();
    ui->mTopLeftLowerFrame->setMinimumHeight(height + 2);
    //overview
    ui->mTopRightLowerFrameLayout->addWidget(mOverview);

    //set up overview
    mOverview->addColumnAt(500, tr("Overview"), true);
    mOverview->setRowCount(4);
    mOverview->setCellContent(0, 0, "hello");
    mOverview->setCellContent(1, 0, "world");
    mOverview->setCellContent(2, 0, "00000000");
    mOverview->setCellContent(3, 0, "here we will list all control flow transfers");
}

TraceWidget::~TraceWidget()
{
    delete ui;
}

void TraceWidget::updateTraceRegistersView(void* registers)
{
    mGeneralRegs->setRegisters((REGDUMP*)registers);
}

TraceBrowser* TraceWidget::getTraceBrowser()
{
    return mTraceWidget;
}

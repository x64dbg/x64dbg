#include "TraceWidget.h"
#include "ui_TraceWidget.h"
#include "TraceBrowser.h"
#include "TraceInfoBox.h"
#include "TraceFileReader.h"
#include "TraceRegisters.h"
#include "StdTable.h"
#include "CPUInfoBox.h"

TraceWidget::TraceWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::TraceWidget)
{
    ui->setupUi(this);

    mTraceWidget = new TraceBrowser(this);
    mOverview = new StdTable(this);
    mInfo = new TraceInfoBox(this);
    mGeneralRegs = new TraceRegisters(this);
    //disasm
    ui->mTopLeftUpperRightFrameLayout->addWidget(mTraceWidget);
    //registers
    mGeneralRegs->setFixedWidth(1000);
    mGeneralRegs->ShowFPU(true);

    QScrollArea* upperScrollArea = new QScrollArea(this);
    upperScrollArea->setFrameShape(QFrame::NoFrame);
    upperScrollArea->setWidget(mGeneralRegs);
    upperScrollArea->setWidgetResizable(true);

    //upperScrollArea->horizontalScrollBar()->setStyleSheet(ConfigHScrollBarStyle());
    //upperScrollArea->verticalScrollBar()->setStyleSheet(ConfigVScrollBarStyle());

    QPushButton* button_changeview = new QPushButton("", this);
    button_changeview->setStyleSheet("Text-align:left;padding: 4px;padding-left: 10px;");
    connect(button_changeview, SIGNAL(clicked()), mGeneralRegs, SLOT(onChangeFPUViewAction()));
    connect(mTraceWidget, SIGNAL(selectionChanged(unsigned long long)), this, SLOT(traceSelectionChanged(unsigned long long)));
    connect(Bridge::getBridge(), SIGNAL(updateTraceBrowser()), this, SLOT(updateSlot()));

    mGeneralRegs->SetChangeButton(button_changeview);

    ui->mTopRightUpperFrameLayout->addWidget(button_changeview);
    ui->mTopRightUpperFrameLayout->addWidget(upperScrollArea);
    ui->mTopHSplitter->setCollapsible(1, true); // allow collapsing the RegisterView

    //info
    ui->mTopLeftLowerFrameLayout->addWidget(mInfo);
    int height = (mInfo->getRowHeight() + 1) * 4;
    ui->mTopLeftLowerFrame->setMinimumHeight(height + 2);
    ui->mTopHSplitter->setSizes(QList<int>({1000, 1}));
    ui->mTopLeftVSplitter->setSizes(QList<int>({1000, 1}));

    //overview
    ui->mTopRightLowerFrameLayout->addWidget(mOverview);

    //set up overview
    mOverview->addColumnAt(0, "", true);
    mOverview->setShowHeader(false);
    mOverview->setRowCount(4);
    mOverview->setCellContent(0, 0, "hello");
    mOverview->setCellContent(1, 0, "world");
    mOverview->setCellContent(2, 0, "00000000");
    mOverview->setCellContent(3, 0, "here we will list all control flow transfers");
    mOverview->hide();
}

TraceWidget::~TraceWidget()
{
    delete ui;
}

void TraceWidget::traceSelectionChanged(unsigned long long selection)
{
    REGDUMP registers;
    TraceFileReader* traceFile;
    traceFile = mTraceWidget->getTraceFile();
    if(traceFile != nullptr && traceFile->Progress() == 100)
    {
        if(selection < traceFile->Length())
        {
            registers = traceFile->Registers(selection);
            mInfo->update(selection, traceFile, registers);
        }
        else
            memset(&registers, 0, sizeof(registers));
    }
    mGeneralRegs->setRegisters(&registers);
}

void TraceWidget::updateSlot()
{
    auto fileOpened = mTraceWidget->isFileOpened();
    mGeneralRegs->setActive(fileOpened);
    if(!fileOpened)
        mInfo->clear();
}

TraceBrowser* TraceWidget::getTraceBrowser()
{
    return mTraceWidget;
}

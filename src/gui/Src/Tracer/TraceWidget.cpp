#include "TraceWidget.h"
#include "ui_TraceWidget.h"
#include "TraceBrowser.h"
#include "TraceInfoBox.h"
#include "TraceDump.h"
#include "TraceFileReader.h"
#include "TraceRegisters.h"
#include "StdTable.h"
#include "CPUInfoBox.h"

TraceWidget::TraceWidget(Architecture* architecture, const QString & fileName, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::TraceWidget)
{
    ui->setupUi(this);

    mTraceFile = new TraceFileReader(this);
    mTraceFile->Open(fileName);
    mTraceBrowser = new TraceBrowser(mTraceFile, this);
    mOverview = new StdTable(this);
    mInfo = new TraceInfoBox(this);
    mMemoryPage = new TraceFileDumpMemoryPage(this);
    mDump = new TraceDump(architecture, mTraceBrowser, mMemoryPage, this);
    mGeneralRegs = new TraceRegisters(this);
    //disasm
    ui->mTopLeftUpperRightFrameLayout->addWidget(mTraceBrowser);
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
    connect(mTraceBrowser, SIGNAL(selectionChanged(unsigned long long)), this, SLOT(traceSelectionChanged(unsigned long long)));
    connect(Bridge::getBridge(), SIGNAL(updateTraceBrowser()), this, SLOT(updateSlot()));
    connect(mTraceFile, SIGNAL(parseFinished()), this, SLOT(parseFinishedSlot()));
    connect(mTraceBrowser, SIGNAL(closeFile()), this, SLOT(closeFileSlot()));

    mGeneralRegs->SetChangeButton(button_changeview);

    ui->mTopRightUpperFrameLayout->addWidget(button_changeview);
    ui->mTopRightUpperFrameLayout->addWidget(upperScrollArea);
    ui->mTopHSplitter->setCollapsible(1, true); // allow collapsing the RegisterView

    //info
    ui->mTopLeftLowerFrameLayout->addWidget(mInfo);
    int height = mInfo->getHeight();
    ui->mTopLeftLowerFrame->setMinimumHeight(height + 2);

    //dump
    //ui->mTopLeftLowerFrameLayout->addWidget(mDump);
    ui->mBotLeftFrameLayout->addWidget(mDump);

    //overview
    ui->mBotRightFrameLayout->addWidget(mOverview);

    // TODO: set up overview
    mOverview->addColumnAt(0, "", true);
    mOverview->setShowHeader(false);
    mOverview->setRowCount(4);
    mOverview->setCellContent(0, 0, "hello");
    mOverview->setCellContent(1, 0, "world");
    mOverview->setCellContent(2, 0, "00000000");
    mOverview->setCellContent(3, 0, "TODO: Draw call stack here");
    mOverview->hide();
    ui->mTopHSplitter->setSizes(QList<int>({1000, 1}));
    ui->mTopLeftVSplitter->setSizes(QList<int>({1000, 1}));
}

TraceWidget::~TraceWidget()
{
    if(mTraceFile)
    {
        mTraceFile->Close();
        delete mTraceFile;
        mTraceFile = nullptr;
    }
    delete ui;
}

void TraceWidget::traceSelectionChanged(unsigned long long selection)
{
    REGDUMP registers;
    if(mTraceFile != nullptr)
    {
        if(selection < mTraceFile->Length())
        {
            // update registers view
            registers = mTraceFile->Registers(selection);
            mInfo->update(selection, mTraceFile, registers);
            // update dump view
            mTraceFile->buildDumpTo(selection); // TODO: sometimes this can be slow
            mMemoryPage->setDumpObject(mTraceFile->getDump());
            mMemoryPage->setSelectedIndex(selection);
            mDump->reloadData();
        }
        else
            memset(&registers, 0, sizeof(registers));
    }
    else
        mMemoryPage->setDumpObject(nullptr);
    mGeneralRegs->setRegisters(&registers);
}

void TraceWidget::parseFinishedSlot()
{
    duint initialAddress;
    if(mTraceFile->isError())
    {
        SimpleErrorBox(this, tr("Error"), tr("Error when opening trace recording"));
        emit closeFile();
    }
    else if(mTraceFile->Length() > 0)
    {
        if(mTraceFile->HashValue() && DbgIsDebugging())
        {
            if(DbgFunctions()->DbGetHash() != mTraceFile->HashValue())
            {
                SimpleWarningBox(this, tr("Trace file is recorded for another debuggee"),
                                 tr("Checksum is different for current trace file and the debugee. This probably means you have opened a wrong trace file. This trace file is recorded for \"%1\"").arg(mTraceFile->ExePath()));
            }
        }
        const int count = mTraceFile->MemoryAccessCount(0);
        if(count > 0)
        {
            // Display source operand
            duint address[MAX_MEMORY_OPERANDS];
            duint oldMemory[MAX_MEMORY_OPERANDS];
            duint newMemory[MAX_MEMORY_OPERANDS];
            bool isValid[MAX_MEMORY_OPERANDS];
            mTraceFile->MemoryAccessInfo(0, address, oldMemory, newMemory, isValid);
            initialAddress = address[count - 1];
        }
        else
        {
            initialAddress = mTraceFile->Registers(0).regcontext.cip;
        }
        mDump->printDumpAt(initialAddress, false, true, true);
    }
}

//void TraceWidget::openSlot(const QString & fileName)
//{
//    emit mTraceBrowser->openSlot(fileName);
//}

void TraceWidget::updateSlot()
{
    auto fileOpened = mTraceBrowser->isFileOpened();
    mGeneralRegs->setActive(fileOpened);
    if(!fileOpened)
        mInfo->clear();
}

void TraceWidget::closeFileSlot()
{
    emit closeFile();
}

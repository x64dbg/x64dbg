#include "TraceWidget.h"
#include "ui_TraceWidget.h"
#include "TraceBrowser.h"
#include "TraceInfoBox.h"
#include "TraceDump.h"
#include "TraceStack.h"
#include "TraceFileReader.h"
#include "TraceRegisters.h"
#include "TraceXrefBrowseDialog.h"
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
    mInfo = new TraceInfoBox(this);
    if(!Config()->getBool("Gui", "DisableTraceDump"))
    {
        mMemoryPage = new TraceFileDumpMemoryPage(mTraceFile->getDump(), this);
        mDump = new TraceDump(architecture, mTraceBrowser, mMemoryPage, this);
        mStack = new TraceStack(architecture, mTraceBrowser, mMemoryPage, this);
        mXrefDlg = nullptr;
    }
    else
    {
        mMemoryPage = nullptr;
        mDump = nullptr;
        mStack = nullptr;
        mXrefDlg = nullptr;
    }
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

    if(mDump)
    {
        //dump
        ui->mBotLeftFrameLayout->addWidget(mDump);
        connect(mDump, SIGNAL(xrefSignal(duint)), this, SLOT(xrefSlot(duint)));

        //stack
        ui->mBotRightFrameLayout->addWidget(mStack);
        connect(mStack, SIGNAL(xrefSignal(duint)), this, SLOT(xrefSlot(duint)));
    }

    ui->mTopHSplitter->setSizes(QList<int>({1000, 1}));
    ui->mTopLeftVSplitter->setSizes(QList<int>({1000, 1}));

    mTraceBrowser->setAccessibleName(tr("Disassembly"));
    upperScrollArea->setAccessibleName(tr("Registers"));
    if(mDump)
    {
        mDump->setAccessibleName(tr("Dump"));
        mStack->setAccessibleName(tr("Stack"));
    }
    mInfo->setAccessibleName(tr("InfoBox"));
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
            if(mDump)
            {
                mTraceFile->buildDumpTo(selection); // TODO: sometimes this can be slow // TODO: Is it a good idea to build dump index just when opening the file?
                mMemoryPage->setSelectedIndex(selection);
                mDump->reloadData();
                mStack->reloadData();
            }
        }
        else
            memset(&registers, 0, sizeof(registers));
    }
    mGeneralRegs->setRegisters(&registers);
}

void TraceWidget::xrefSlot(duint addr)
{
    if(!mXrefDlg)
        mXrefDlg = new TraceXrefBrowseDialog(this);
    mXrefDlg->setup(mTraceBrowser->getInitialSelection(), addr, mTraceFile, [this](duint addr)
    {
        mTraceBrowser->gotoIndexSlot(addr);
    });
    mXrefDlg->showNormal();
}

void TraceWidget::parseFinishedSlot()
{
    duint initialAddress;
    QString reason;
    if(mTraceFile->isError(reason))
    {
        SimpleErrorBox(this, tr("Error"), tr("Error when opening trace recording (reason: %1)").arg(reason));
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
        if(mDump)
        {
            // Setting the initial address of dump view
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
                // No memory operands, so display opcode instead
                initialAddress = mTraceFile->Registers(0).regcontext.cip;
            }
            mDump->printDumpAt(initialAddress, false, true, true);
            // Setting the initial address of stack view
            mStack->printDumpAt(mTraceFile->Registers(0).regcontext.csp, false, true, true);
        }
        mGeneralRegs->setActive(true);
    }
}

void TraceWidget::closeFileSlot()
{
    emit closeFile();
}

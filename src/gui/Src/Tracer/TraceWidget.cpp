#include <QMessageBox>
#include <QPushButton>

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

    setCursor(QCursor(Qt::CursorShape::WaitCursor));
    mTraceFile = new TraceFileReader(this);
    mTraceFile->Open(fileName);
    mTraceBrowser = new TraceBrowser(mTraceFile, this);
    mInfo = new TraceInfoBox(this);
    mArchitecture = architecture;
    if(Config()->getBool("Gui", "AutoTraceDump"))
    {
        mTraceFile->getDump()->setEnabled();
        mMemoryPage = new TraceFileDumpMemoryPage(mTraceFile->getDump(), this);
        mDump = new TraceDump(architecture, this, mMemoryPage);
        mStack = new TraceStack(architecture, this, mMemoryPage);
        mLoadDump = nullptr;
    }
    else
    {
        mMemoryPage = nullptr;
        mDump = nullptr;
        mStack = nullptr;
        mLoadDump = new QPushButton(tr("Load dump"), this);
        connect(mLoadDump, SIGNAL(clicked()), this, SLOT(loadDump()));
    }
    mXrefDlg = nullptr;
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
    connect(mTraceBrowser, SIGNAL(selectionChanged(TRACEINDEX)), this, SLOT(traceSelectionChanged(TRACEINDEX)));
    connect(mTraceBrowser, SIGNAL(displayLogWidget()), this, SLOT(displayLogWidgetSlot()));
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

    connect(mTraceBrowser, SIGNAL(xrefSignal(duint)), this, SLOT(xrefSlot(duint)));
    if(mDump != nullptr)
    {
        //dump
        ui->mBotLeftFrameLayout->addWidget(mDump);
        connect(mDump, SIGNAL(xrefSignal(duint)), this, SLOT(xrefSlot(duint)));

        //stack
        ui->mBotRightFrameLayout->addWidget(mStack);
        connect(mStack, SIGNAL(xrefSignal(duint)), this, SLOT(xrefSlot(duint)));
    }
    else
    {
        ui->mBotLeftFrameLayout->addWidget(mLoadDump);
    }

    ui->mTopHSplitter->setSizes(QList<int>({1000, 1}));
    ui->mTopLeftVSplitter->setSizes(QList<int>({1000, 1}));

    mTraceBrowser->setAccessibleName(tr("Disassembly"));
    upperScrollArea->setAccessibleName(tr("Registers"));
    if(mDump != nullptr)
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

void TraceWidget::traceSelectionChanged(TRACEINDEX selection)
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
            if(mDump != nullptr)
            {
                mTraceFile->buildDumpTo(selection); // TODO: sometimes this can be slow
                mMemoryPage->setSelectedIndex(selection);
                mDump->reloadData();
                mStack->reloadData();
            }
        }
        else
            memset(&registers, 0, sizeof(registers));
    }
    else
        memset(&registers, 0, sizeof(registers));
    mGeneralRegs->setRegisters(&registers);
}

void TraceWidget::xrefSlot(duint addr)
{
    if(!mDump)
        if(!loadDumpFully())
            return;
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
    QString reason;
    setCursor(QCursor(Qt::CursorShape::ArrowCursor));
    if(mTraceFile->isError(reason))
    {
        SimpleErrorBox(this, tr("Error"), tr("Error when opening trace recording (reason: %1)").arg(reason));
        emit closeFile();
        return;
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
        if(mDump != nullptr)
        {
            setupDumpInitialAddresses(0);
        }
    }
    mGeneralRegs->setActive(true);
}

void TraceWidget::closeFileSlot()
{
    emit closeFile();
}

void TraceWidget::displayLogWidgetSlot()
{
    emit displayLogWidget();
}

bool TraceWidget::loadDump()
{
    // The dump should not be loaded at this point
    if(mDump != nullptr)
        return true;

    // Warn the user
    auto fileSize = mTraceFile->FileSize();
    auto estimatedGb = fileSize / 1024.0 / 1024.0 / 1024.0 * 10.0;
    if(estimatedGb > 0.2)
    {
        auto message = tr("Enabling the trace dump can consume a lot of memory (max ~%1GiB for this trace) and freeze x64dbg for prolonged periods of time. This feature is still experimental, please report any bugs you encounter.").arg(estimatedGb, 0, 'f', 2);
        QMessageBox msg(QMessageBox::Warning, tr("Warning"), message, QMessageBox::Ok | QMessageBox::Cancel, this);
        msg.setWindowIcon(DIcon("exclamation"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Cancel)
            return false;
    }

    mTraceFile->getDump()->setEnabled();
    mMemoryPage = new TraceFileDumpMemoryPage(mTraceFile->getDump(), this);
    auto selection = mTraceBrowser->getInitialSelection();
    mTraceFile->buildDumpTo(selection); // TODO: sometimes this can be slow
    mMemoryPage->setSelectedIndex(selection);
    mDump = new TraceDump(mArchitecture, this, mMemoryPage);
    mStack = new TraceStack(mArchitecture, this, mMemoryPage);

    //dump
    ui->mBotLeftFrameLayout->removeWidget(mLoadDump);
    delete mLoadDump;
    mLoadDump = nullptr;
    ui->mBotLeftFrameLayout->addWidget(mDump);
    connect(mDump, SIGNAL(xrefSignal(duint)), this, SLOT(xrefSlot(duint)));
    mDump->setAccessibleName(tr("Dump"));

    //stack
    ui->mBotRightFrameLayout->addWidget(mStack);
    connect(mStack, SIGNAL(xrefSignal(duint)), this, SLOT(xrefSlot(duint)));
    mStack->setAccessibleName(tr("Stack"));

    setupDumpInitialAddresses(selection);
    return true;
}

bool TraceWidget::loadDumpFully()
{
    if(mTraceFile->Length() == 0)
        return false;

    if(!mTraceFile->getDump()->isEnabled())
        if(!loadDump())
            return false;

    // Fully build dump index
    mTraceFile->buildDumpTo(mTraceFile->Length() - 1);
    return true;
}

void TraceWidget::setupDumpInitialAddresses(TRACEINDEX selection)
{
    // Setting the initial address of dump view
    duint initialAddress;
    const int count = mTraceFile->MemoryAccessCount(selection);
    if(count > 0)
    {
        // Display source operand
        duint address[MAX_MEMORY_OPERANDS];
        duint oldMemory[MAX_MEMORY_OPERANDS];
        duint newMemory[MAX_MEMORY_OPERANDS];
        bool isValid[MAX_MEMORY_OPERANDS];
        mTraceFile->MemoryAccessInfo(selection, address, oldMemory, newMemory, isValid);
        initialAddress = address[count - 1];
    }
    else
    {
        // No memory operands, so display opcode instead
        initialAddress = mTraceFile->Registers(selection).regcontext.cip;
    }
    mDump->printDumpAt(initialAddress, false, true, true);
    // Setting the initial address of stack view
    mStack->printDumpAt(mTraceFile->Registers(selection).regcontext.csp, false, true, true);
}

/**
 * @brief TraceWidget::addFollowMenuItem Add a follow action to the menu
 * @param menu The menu to which the follow action adds
 * @param name The user-friendly name of the action
 * @param value The VA of the address
 */
void TraceWidget::addFollowMenuItem(QMenu* menu, QString name, duint value)
{
    foreach(QAction* action, menu->actions()) //check for duplicate action
        if(action->text() == name)
            return;
    QAction* newAction = new QAction(name, menu);
    menu->addAction(newAction);
    newAction->setObjectName(ToPtrString(value));
    connect(newAction, SIGNAL(triggered()), this, SLOT(followActionSlot()));
}

/**
 * @brief TraceWidget::setupFollowMenu Set up a follow menu.
 * @param menu The menu to create
 * @param va The selected VA
 */
void TraceWidget::setupFollowMenu(QMenu* menu)
{
    const TraceFileDump* traceDump = mTraceFile->getDump();
    if(!traceDump->isEnabled())
    {
        QAction* newAction = new QAction(tr("Load dump"), menu);
        connect(newAction, SIGNAL(triggered()), this, SLOT(loadDump()));
        menu->addAction(newAction);
        return;
    }

    //add follow actions
    TRACEINDEX selection = getTraceBrowser()->getInitialSelection();
    REGDUMP registers = mTraceFile->Registers(selection);
    duint va = registers.regcontext.cip;
    Zydis zydis;
    unsigned char opcode[16];
    int opsize;
    mTraceFile->OpCode(selection, opcode, &opsize);
    duint MemoryAddress[MAX_MEMORY_OPERANDS];
    duint MemoryOldContent[MAX_MEMORY_OPERANDS];
    duint MemoryNewContent[MAX_MEMORY_OPERANDS];
    bool MemoryIsValid[MAX_MEMORY_OPERANDS];
    int MemoryOperandsCount;
    MemoryOperandsCount = mTraceFile->MemoryAccessCount(selection);
    if(MemoryOperandsCount > 0)
        mTraceFile->MemoryAccessInfo(selection, MemoryAddress, MemoryOldContent, MemoryNewContent, MemoryIsValid);

    //most basic follow action
    addFollowMenuItem(menu, tr("&Selected Address"), va);
    if(zydis.Disassemble(va, opcode, opsize))
    {
        for(uint8_t opindex = 0; opindex < zydis.OpCount(); opindex++)
        {
            size_t value = zydis.ResolveOpValue(opindex, [&registers](ZydisRegister reg)
            {
                return resolveZydisRegister(registers, reg);
            });

            if(zydis[opindex].type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                if(zydis[opindex].size == sizeof(void*) * 8)
                {
                    if(traceDump->isValidReadPtr(value))
                    {
                        addFollowMenuItem(menu, tr("&Address: ") + QString::fromStdString(zydis.OperandText(opindex)), value);
                    }
                    for(uint8_t memaccessindex = 0; memaccessindex < MemoryOperandsCount; memaccessindex++)
                    {
                        if(MemoryAddress[memaccessindex] == value)
                        {
                            if(traceDump->isValidReadPtr(MemoryOldContent[memaccessindex]))
                            {
                                if(MemoryOldContent[memaccessindex] != MemoryNewContent[memaccessindex])
                                {
                                    addFollowMenuItem(menu, tr("&Old value: ") + ToPtrString(MemoryOldContent[memaccessindex]), MemoryOldContent[memaccessindex]);
                                }
                                else
                                {
                                    addFollowMenuItem(menu, tr("&Value: ") + ToPtrString(MemoryOldContent[memaccessindex]), MemoryOldContent[memaccessindex]);
                                    break;
                                }
                            }
                            if(traceDump->isValidReadPtr(MemoryNewContent[memaccessindex]))
                            {
                                addFollowMenuItem(menu, tr("&New value: ") + ToPtrString(MemoryNewContent[memaccessindex]), MemoryNewContent[memaccessindex]);
                            }
                            break;
                        }
                    }
                }
            }
            else if(zydis[opindex].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
            {
                if(traceDump->isValidReadPtr(value))
                {
                    addFollowMenuItem(menu, tr("&Constant: ") + QString::fromStdString(zydis.OperandText(opindex)), value);
                }
            }
        }
    }
}

/**
 * @brief TraceWidget::followActionSlot Called when follow action is clicked
 */
void TraceWidget::followActionSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    duint data;
#ifdef _WIN64
    data = action->objectName().toULongLong(nullptr, 16);
#else
    data = action->objectName().toULong(nullptr, 16);
#endif //_WIN64
    mDump->printDumpAt(data, true, true, true);
}

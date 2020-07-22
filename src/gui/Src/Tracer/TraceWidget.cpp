#include "TraceWidget.h"
#include "ui_TraceWidget.h"
#include "TraceBrowser.h"
#include "TraceFileReader.h"
#include "TraceRegisters.h"
#include "StdTable.h"

TraceWidget::TraceWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::TraceWidget)
{
    ui->setupUi(this);

    mTraceWidget = new TraceBrowser(this);
    mOverview = new StdTable(this);
    mInfo = new StdTable(this);
    mGeneralRegs = new TraceRegisters(this);
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
    connect(mTraceWidget, SIGNAL(selectionChanged(unsigned long long)), this, SLOT(traceSelectionChanged(unsigned long long)));
    connect(Bridge::getBridge(), SIGNAL(updateTraceBrowser()), this, SLOT(updateSlot()));

    mGeneralRegs->SetChangeButton(button_changeview);

    ui->mTopRightUpperFrameLayout->addWidget(button_changeview);
    ui->mTopRightUpperFrameLayout->addWidget(upperScrollArea);
    //info
    ui->mTopLeftLowerFrameLayout->addWidget(mInfo);
    int height = (mInfo->getRowHeight() + 1) * 4;
    ui->mTopLeftLowerFrame->setMinimumHeight(height + 2);
    ui->mTopHSplitter->setSizes(QList<int>({1000, 1}));
    ui->mTopLeftVSplitter->setSizes(QList<int>({1000, 1}));

    mInfo->addColumnAt(0, "", true);
    mInfo->setShowHeader(false);
    mInfo->setRowCount(4);
    mInfo->setCellContent(0, 0, QString());
    mInfo->setCellContent(1, 0, QString());
    mInfo->setCellContent(2, 0, QString());
    mInfo->setCellContent(3, 0, QString());
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
            updateInfobox(selection, traceFile, registers);
        }
        else
            memset(&registers, 0, sizeof(registers));
    }
    mGeneralRegs->setRegisters(&registers);
}

void TraceWidget::updateSlot()
{
    mGeneralRegs->setActive(mTraceWidget->isFileOpened());
}

TraceBrowser* TraceWidget::getTraceBrowser()
{
    return mTraceWidget;
}

void TraceWidget::updateInfobox(unsigned long long selection, TraceFileReader* traceFile, const REGDUMP & registers)
{
    int infoline = 0;
    Zydis zydis;
    unsigned char opcode[16];
    QString line;
    int opsize;
    traceFile->OpCode(selection, opcode, &opsize);
    mInfo->setRowCount(4);
    mInfo->setCellContent(0, 0, QString());
    mInfo->setCellContent(1, 0, QString());
    mInfo->setCellContent(2, 0, QString());
    mInfo->setCellContent(3, 0, QString());
    auto resolveRegValue = [registers](ZydisRegister regname)
    {
        switch(regname)
        {
#ifdef _WIN64
        case ZYDIS_REGISTER_RAX:
            return registers.regcontext.cax;
        case ZYDIS_REGISTER_RCX:
            return registers.regcontext.ccx;
        case ZYDIS_REGISTER_RDX:
            return registers.regcontext.cdx;
        case ZYDIS_REGISTER_RBX:
            return registers.regcontext.cbx;
        case ZYDIS_REGISTER_RSP:
            return registers.regcontext.csp;
        case ZYDIS_REGISTER_RBP:
            return registers.regcontext.cbp;
        case ZYDIS_REGISTER_RSI:
            return registers.regcontext.csi;
        case ZYDIS_REGISTER_RDI:
            return registers.regcontext.cdi;
        case ZYDIS_REGISTER_R8:
            return registers.regcontext.r8;
        case ZYDIS_REGISTER_R9:
            return registers.regcontext.r9;
        case ZYDIS_REGISTER_R10:
            return registers.regcontext.r10;
        case ZYDIS_REGISTER_R11:
            return registers.regcontext.r11;
        case ZYDIS_REGISTER_R12:
            return registers.regcontext.r12;
        case ZYDIS_REGISTER_R13:
            return registers.regcontext.r13;
        case ZYDIS_REGISTER_R14:
            return registers.regcontext.r14;
        case ZYDIS_REGISTER_R15:
            return registers.regcontext.r15;
        case ZYDIS_REGISTER_R8D:
            return registers.regcontext.r8 & 0xFFFFFFFF;
        case ZYDIS_REGISTER_R9D:
            return registers.regcontext.r9 & 0xFFFFFFFF;
        case ZYDIS_REGISTER_R10D:
            return registers.regcontext.r10 & 0xFFFFFFFF;
        case ZYDIS_REGISTER_R11D:
            return registers.regcontext.r11 & 0xFFFFFFFF;
        case ZYDIS_REGISTER_R12D:
            return registers.regcontext.r12 & 0xFFFFFFFF;
        case ZYDIS_REGISTER_R13D:
            return registers.regcontext.r13 & 0xFFFFFFFF;
        case ZYDIS_REGISTER_R15D:
            return registers.regcontext.r15 & 0xFFFFFFFF;
        case ZYDIS_REGISTER_R8W:
            return registers.regcontext.r8 & 0xFFFF;
        case ZYDIS_REGISTER_R9W:
            return registers.regcontext.r9 & 0xFFFF;
        case ZYDIS_REGISTER_R10W:
            return registers.regcontext.r10 & 0xFFFF;
        case ZYDIS_REGISTER_R11W:
            return registers.regcontext.r11 & 0xFFFF;
        case ZYDIS_REGISTER_R12W:
            return registers.regcontext.r12 & 0xFFFF;
        case ZYDIS_REGISTER_R13W:
            return registers.regcontext.r13 & 0xFFFF;
        case ZYDIS_REGISTER_R15W:
            return registers.regcontext.r15 & 0xFFFF;
        case ZYDIS_REGISTER_R8B:
            return registers.regcontext.r8 & 0xFF;
        case ZYDIS_REGISTER_R9B:
            return registers.regcontext.r9 & 0xFF;
        case ZYDIS_REGISTER_R10B:
            return registers.regcontext.r10 & 0xFF;
        case ZYDIS_REGISTER_R11B:
            return registers.regcontext.r11 & 0xFF;
        case ZYDIS_REGISTER_R12B:
            return registers.regcontext.r12 & 0xFF;
        case ZYDIS_REGISTER_R13B:
            return registers.regcontext.r13 & 0xFF;
        case ZYDIS_REGISTER_R15B:
            return registers.regcontext.r15 & 0xFF;
#endif //_WIN64
        case ZYDIS_REGISTER_EAX:
            return registers.regcontext.cax & 0xFFFFFFFF;
        case ZYDIS_REGISTER_ECX:
            return registers.regcontext.ccx & 0xFFFFFFFF;
        case ZYDIS_REGISTER_EDX:
            return registers.regcontext.cdx & 0xFFFFFFFF;
        case ZYDIS_REGISTER_EBX:
            return registers.regcontext.cbx & 0xFFFFFFFF;
        case ZYDIS_REGISTER_ESP:
            return registers.regcontext.csp & 0xFFFFFFFF;
        case ZYDIS_REGISTER_EBP:
            return registers.regcontext.cbp & 0xFFFFFFFF;
        case ZYDIS_REGISTER_ESI:
            return registers.regcontext.csi & 0xFFFFFFFF;
        case ZYDIS_REGISTER_EDI:
            return registers.regcontext.cdi & 0xFFFFFFFF;
        case ZYDIS_REGISTER_AX:
            return registers.regcontext.cax & 0xFFFF;
        case ZYDIS_REGISTER_CX:
            return registers.regcontext.ccx & 0xFFFF;
        case ZYDIS_REGISTER_DX:
            return registers.regcontext.cdx & 0xFFFF;
        case ZYDIS_REGISTER_BX:
            return registers.regcontext.cbx & 0xFFFF;
        case ZYDIS_REGISTER_SP:
            return registers.regcontext.csp & 0xFFFF;
        case ZYDIS_REGISTER_BP:
            return registers.regcontext.cbp & 0xFFFF;
        case ZYDIS_REGISTER_SI:
            return registers.regcontext.csi & 0xFFFF;
        case ZYDIS_REGISTER_DI:
            return registers.regcontext.cdi & 0xFFFF;
        case ZYDIS_REGISTER_AL:
            return registers.regcontext.cax & 0xFF;
        case ZYDIS_REGISTER_CL:
            return registers.regcontext.ccx & 0xFF;
        case ZYDIS_REGISTER_DL:
            return registers.regcontext.cdx & 0xFF;
        case ZYDIS_REGISTER_BL:
            return registers.regcontext.cbx & 0xFF;
        case ZYDIS_REGISTER_AH:
            return (registers.regcontext.cax & 0xFF00) >> 8;
        case ZYDIS_REGISTER_CH:
            return (registers.regcontext.ccx & 0xFF00) >> 8;
        case ZYDIS_REGISTER_DH:
            return (registers.regcontext.cdx & 0xFF00) >> 8;
        case ZYDIS_REGISTER_BH:
            return (registers.regcontext.cbx & 0xFF00) >> 8;
        default:
            return static_cast<size_t>(0);
        }
    };
    if(zydis.Disassemble(registers.regcontext.cip, opcode, opsize))
    {
        int opindex;
        //Jumps
        if(zydis.IsBranchType(Zydis::BTCondJmp))
        {
            if(zydis.IsBranchGoingToExecute(registers.regcontext.eflags, registers.regcontext.ccx))
            {
                line = tr("Jump is taken");
            }
            else
            {
                line = tr("Jump is not taken");
            }
            mInfo->setCellContent(infoline, 0, line);
            infoline++;
        }
        //Operands
        for(opindex = 0; opindex < zydis.OpCount(); opindex++)
        {
            line.clear();
            size_t value = zydis.ResolveOpValue(opindex, resolveRegValue);
            if(zydis[opindex].type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                const char* memsize = zydis.MemSizeName(zydis[opindex].size / 8);
                if(memsize != nullptr)
                {
                    line += memsize;
                }
                line += " ptr ";
                line += zydis.RegName(zydis[opindex].mem.segment);
                line += ":[";
                line += ToPtrString(value);
                line += "]";
                mInfo->setCellContent(infoline, 0, line);
                infoline++;
            }
            else if(zydis[opindex].type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                line += zydis.RegName(zydis[opindex].reg.value);
                line += " = ";
                line += ToPtrString(value);
                mInfo->setCellContent(infoline, 0, line);
                infoline++;
            }
        }
    }
    mInfo->reloadData();
}

#include "TraceInfoBox.h"
#include "TraceWidget.h"
#include "TraceDump.h"
#include "TraceBrowser.h"
#include "CPUInfoBox.h"
#include "zydis_wrapper.h"

TraceInfoBox::TraceInfoBox(TraceWidget* parent) : StdTable(parent), mParent(parent)
{
    setWindowTitle("TraceInfoBox");
    enableMultiSelection(false);
    setShowHeader(false);
    setRowCount(4);
    addColumnAt(0, "", true);
    setCellContent(0, 0, "");
    setCellContent(1, 0, "");
    setCellContent(2, 0, "");
    setCellContent(3, 0, "");
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setShowHeader(false);
    clear();
    setMinimumHeight((getRowHeight() + 1) * 4);

    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    mCurAddr = 0;

    // Deselect any row (visual reasons only)
    setSingleSelection(-1);
}

TraceInfoBox::~TraceInfoBox()
{

}

void TraceInfoBox::update(TRACEINDEX selection, TraceFileReader* traceFile, const REGDUMP & registers)
{
    duint infoline = 0;
    Zydis zydis;
    unsigned char opcode[16];
    QString line;
    int opsize;
    traceFile->OpCode(selection, opcode, &opsize);
    clear();
    duint MemoryAddress[MAX_MEMORY_OPERANDS];
    duint MemoryOldContent[MAX_MEMORY_OPERANDS];
    duint MemoryNewContent[MAX_MEMORY_OPERANDS];
    bool MemoryIsValid[MAX_MEMORY_OPERANDS];
    int MemoryOperandsCount;
    MemoryOperandsCount = traceFile->MemoryAccessCount(selection);
    if(MemoryOperandsCount > 0)
        traceFile->MemoryAccessInfo(selection, MemoryAddress, MemoryOldContent, MemoryNewContent, MemoryIsValid);
    mCurAddr = registers.regcontext.cip;
    if(zydis.Disassemble(mCurAddr, opcode, opsize))
    {
        uint8_t opindex;
        int memaccessindex;
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
            setCellContent(infoline, 0, line);
            infoline++;
        }
        //Operands
        QString registerLine, memoryLine;
        for(opindex = 0; opindex < zydis.OpCount(); opindex++)
        {
            size_t value = zydis.ResolveOpValue(opindex, [&registers](ZydisRegister reg)
            {
                return resolveZydisRegister(registers, reg);
            });
            if(zydis[opindex].type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                if(!memoryLine.isEmpty())
                    memoryLine += ", ";
                const char* memsize = zydis.MemSizeName(zydis[opindex].size / 8);
                if(memsize != nullptr)
                {
                    memoryLine += memsize;
                }
                else
                {
                    memoryLine += QString("m%1").arg(zydis[opindex].size / 8);
                }
                memoryLine += " ptr ";
                memoryLine += zydis.RegName(zydis[opindex].mem.segment);
                memoryLine += ":[";
                memoryLine += ToPtrString(value);
                memoryLine += "]";
                if(zydis[opindex].size == 64 && zydis.getVectorElementType(opindex) == Zydis::VETFloat64)
                {
                    // Double precision
#ifdef _WIN64
                    for(memaccessindex = 0; memaccessindex < MemoryOperandsCount; memaccessindex++)
                    {
                        if(MemoryAddress[memaccessindex] == value)
                        {
                            memoryLine += "= ";
                            memoryLine += ToDoubleString(&MemoryOldContent[memaccessindex]);
                            memoryLine += " -> ";
                            memoryLine += ToDoubleString(&MemoryNewContent[memaccessindex]);
                            break;
                        }
                    }
#else
                    // On 32-bit platform it is saved as 2 memory accesses.
                    for(memaccessindex = 0; memaccessindex < MemoryOperandsCount - 1; memaccessindex++)
                    {
                        if(MemoryAddress[memaccessindex] == value && MemoryAddress[memaccessindex + 1] == value + 4)
                        {
                            double dblval;
                            memoryLine += "= ";
                            memcpy(&dblval, &MemoryOldContent[memaccessindex], 4);
                            memcpy(((char*)&dblval) + 4, &MemoryOldContent[memaccessindex + 1], 4);
                            memoryLine += ToDoubleString(&dblval);
                            memoryLine += " -> ";
                            memcpy(&dblval, &MemoryNewContent[memaccessindex], 4);
                            memcpy(((char*)&dblval) + 4, &MemoryNewContent[memaccessindex + 1], 4);
                            memoryLine += ToDoubleString(&dblval);
                            break;
                        }
                    }
#endif //_WIN64
                }
                else if(zydis[opindex].size == 32 && zydis.getVectorElementType(opindex) == Zydis::VETFloat32)
                {
                    // Single precision
                    for(memaccessindex = 0; memaccessindex < MemoryOperandsCount; memaccessindex++)
                    {
                        if(MemoryAddress[memaccessindex] == value)
                        {
                            memoryLine += "= ";
                            memoryLine += ToFloatString(&MemoryOldContent[memaccessindex]);
                            memoryLine += " -> ";
                            memoryLine += ToFloatString(&MemoryNewContent[memaccessindex]);
                            break;
                        }
                    }
                }
                else if(zydis[opindex].size <= sizeof(void*) * 8)
                {
                    // Handle the most common case (ptr-sized)
                    duint mask;
                    if(zydis[opindex].size < sizeof(void*) * 8)
                        mask = ((duint)1 << zydis[opindex].size) - 1;
                    else
                        mask = ~(duint)0;
                    for(memaccessindex = 0; memaccessindex < MemoryOperandsCount; memaccessindex++)
                    {
                        if(MemoryAddress[memaccessindex] == value)
                        {
                            memoryLine += "=";
                            memoryLine += ToHexString(MemoryOldContent[memaccessindex] & mask);
                            memoryLine += " -> ";
                            memoryLine += ToHexString(MemoryNewContent[memaccessindex] & mask);
                            break;
                        }
                    }
                }
            }
            else if(zydis[opindex].type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                const auto registerName = zydis[opindex].reg.value;
                if(!registerLine.isEmpty())
                    registerLine += ", ";
                registerLine += zydis.RegName(registerName);
                registerLine += " = ";
                // Special treatment for FPU registers
                if(registerName >= ZYDIS_REGISTER_ST0 && registerName <= ZYDIS_REGISTER_ST7)
                {
                    // x87 FPU
                    registerLine += ToLongDoubleString(&registers.x87FPURegisters[(registers.x87StatusWordFields.TOP + registerName - ZYDIS_REGISTER_ST0) & 7].data);
                }
                else if(registerName >= ZYDIS_REGISTER_XMM0 && registerName <= ArchValue(ZYDIS_REGISTER_XMM7, ZYDIS_REGISTER_XMM15))
                {
                    registerLine += CPUInfoBox::formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[registerName - ZYDIS_REGISTER_XMM0], 16), zydis.getVectorElementType(opindex));
                }
                else if(registerName >= ZYDIS_REGISTER_YMM0 && registerName <= ArchValue(ZYDIS_REGISTER_YMM7, ZYDIS_REGISTER_YMM15))
                {
                    registerLine += CPUInfoBox::formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[registerName - ZYDIS_REGISTER_XMM0], 32), zydis.getVectorElementType(opindex));
                }
                else
                {
                    // GPR
                    registerLine += ToPtrString(value);
                }
            }
        }
        if(!registerLine.isEmpty())
        {
            setCellContent(infoline, 0, registerLine);
            infoline++;
        }
        if(!memoryLine.isEmpty())
        {
            setCellContent(infoline, 0, memoryLine);
            infoline++;
        }
        DWORD tid;
        tid = traceFile->ThreadId(selection);
        line = QString("ThreadID: %1").arg(ConfigBool("Gui", "PidTidInHex") ? ToHexString(tid) : QString::number(tid));
        setCellContent(3, 0, line);
    }
    reloadData();
}

void TraceInfoBox::clear()
{
    setRowCount(4);
    for(duint i = 0; i < 4; i++)
        setCellContent(i, 0, QString());
    reloadData();
}

void TraceInfoBox::setupContextMenu()
{
    mCopyLineAction = makeAction(tr("Copy Line"), SLOT(copyLineSlot()));
    setupShortcuts();
}

int TraceInfoBox::getHeight()
{
    return ((getRowHeight() + 1) * 4);
}

/**
 * @brief TraceInfoBox::addFollowMenuItem Add a follow action to the menu
 * @param menu The menu to which the follow action adds
 * @param name The user-friendly name of the action
 * @param value The VA of the address
 */
void TraceInfoBox::addFollowMenuItem(QMenu* menu, QString name, duint value)
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
 * @brief TraceInfoBox::setupFollowMenu Set up a follow menu.
 * @param menu The menu to create
 * @param va The selected VA
 */
void TraceInfoBox::setupFollowMenu(QMenu* menu, duint va)
{
    TraceFileReader* traceFile = mParent->getTraceFile();
    const TraceFileDump* traceDump = traceFile->getDump();
    if(!traceDump->isEnabled())
        return;
    //most basic follow action
    addFollowMenuItem(menu, tr("&Selected Address"), va);

    //add follow actions
    duint selection = mParent->getTraceBrowser()->getInitialSelection();
    Zydis zydis;
    unsigned char opcode[16];
    int opsize;
    traceFile->OpCode(selection, opcode, &opsize);
    duint MemoryAddress[MAX_MEMORY_OPERANDS];
    duint MemoryOldContent[MAX_MEMORY_OPERANDS];
    duint MemoryNewContent[MAX_MEMORY_OPERANDS];
    bool MemoryIsValid[MAX_MEMORY_OPERANDS];
    int MemoryOperandsCount;
    MemoryOperandsCount = traceFile->MemoryAccessCount(selection);
    if(MemoryOperandsCount > 0)
        traceFile->MemoryAccessInfo(selection, MemoryAddress, MemoryOldContent, MemoryNewContent, MemoryIsValid);
    REGDUMP registers = traceFile->Registers(selection);
    if(zydis.Disassemble(mCurAddr, opcode, opsize))
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

void TraceInfoBox::contextMenuSlot(QPoint pos)
{
    QMenu menu(this); //create context menu
    QMenu followMenu(tr("&Follow in Dump"), this);
    followMenu.setIcon(DIcon("dump"));
    setupFollowMenu(&followMenu, mCurAddr);
    menu.addMenu(&followMenu);
    QMenu copyMenu(tr("&Copy"), this);
    setupCopyMenu(&copyMenu);
    menu.addMenu(&copyMenu);
    menu.exec(mapToGlobal(pos)); //execute context menu
}

/**
 * @brief TraceInfoBox::followActionSlot Called when follow action is clicked
 */
void TraceInfoBox::followActionSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    duint data;
#ifdef _WIN64
    data = action->objectName().toULongLong(nullptr, 16);
#else
    data = action->objectName().toULong(nullptr, 16);
#endif //_WIN64
    mParent->getTraceDump()->printDumpAt(data, true, true, true);
}

void TraceInfoBox::setupShortcuts()
{
    mCopyLineAction->setShortcut(ConfigShortcut("ActionCopyLine"));
    addAction(mCopyLineAction);
}

#include "historycontext.h"
#include "disasm_helper.h"
#include "memory.h"
#include "console.h"
#include "watch.h"
#include "threading.h"
#include "cmd-watch-control.h"
#include "debugger.h"
#include <deque>

/**
 * @brief The class used to hold history context.
**/
class HistoryContext
{
public:
    HistoryContext();
    void restore();

protected:
    TITAN_ENGINE_CONTEXT_t registers;
    struct ChangedData
    {
        duint addr;
        char oldvalue[32];
    };
    std::vector<ChangedData> ChangedLocation;
    bool invalid;
};

static const duint HistoryMaxCount = 4096;
static std::deque<HistoryContext> history;

//This will capture the current instruction
HistoryContext::HistoryContext()
{
    DISASM_INSTR instruction;
    if(GetFullContextDataEx(hActiveThread, &registers) && MemIsValidReadPtr(registers.cip))
    {
        disasmget(registers.cip, &instruction);
        if(!(memcmp(instruction.instruction, "nop ", 4) == 0 || memcmp(instruction.instruction, "lea ", 4) == 0))
            // do not process the memory operands of these instructions, because these instructions do not write to the memory.
        {
            if(memcmp(instruction.instruction, "push", 4) == 0 || memcmp(instruction.instruction, "call", 4) == 0)
                // push and call instruction : save previous content on the stack
            {
                ChangedData olddata;
                olddata.addr = registers.csp - sizeof(duint);
                MemRead(olddata.addr, (unsigned char*)olddata.oldvalue, sizeof(duint));
                ChangedLocation.push_back(olddata);
            }
            // TODO: FPU/SSE instructions, ENTER instruction, and possibly other instructions.
            // These instructions can write more than sizeof(duint) bytes.
            for(int i = 0; i < instruction.argcount; i++)
            {
                DISASM_ARG & arg = instruction.arg[i];
                if(arg.type == DISASM_ARGTYPE::arg_memory)
                {
                    ChangedData olddata;
                    olddata.addr = arg.value;
                    memcpy(olddata.oldvalue, &arg.memvalue, sizeof(arg.memvalue));
                    ChangedLocation.push_back(olddata);
                }
            }
        }
        invalid = false;
    }
    else
        invalid = true;
}

void HistoryContext::restore()
{
    if(!invalid)
    {
        for(auto & i : ChangedLocation)
            MemWrite(i.addr, i.oldvalue, sizeof(duint));
        SetFullContextDataEx(hActiveThread, &registers);
        cbCheckWatchdog(0, nullptr);
        DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    }
    else
    {
        HistoryClear();
        dputs(QT_TRANSLATE_NOOP("DBG", "Cannot restore last instruction."));
    }
}

void HistoryRecord()
{
    EXCLUSIVE_ACQUIRE(LockHistory);
    if(history.size() > HistoryMaxCount)
        history.pop_front();
    history.emplace_back(); // the constructor records the context
}

void HistoryRestore()
{
    EXCLUSIVE_ACQUIRE(LockHistory);
    if(!history.empty())
    {
        history.back().restore();
        history.pop_back();
    }
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "History record is empty"));
}

bool HistoryIsEmpty()
{
    SHARED_ACQUIRE(LockHistory);
    return history.empty();
}

void HistoryClear()
{
    EXCLUSIVE_ACQUIRE(LockHistory);
    history.clear();
}

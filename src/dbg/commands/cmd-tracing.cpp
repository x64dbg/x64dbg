#include "cmd-tracing.h"
#include "debugger.h"
#include "historycontext.h"
#include "threading.h"
#include "module.h"
#include "console.h"
#include "cmd-debug-control.h"
#include "value.h"

extern std::vector<std::pair<duint, duint>> RunToUserCodeBreakpoints;

static CMDRESULT cbDebugConditionalTrace(void* callBack, bool stepOver, int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not enough arguments"));
        return STATUS_ERROR;
    }
    if(dbgtraceactive())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Trace already active"));
        return STATUS_ERROR;
    }
    duint maxCount = 50000;
    if(argc > 2 && !valfromstring(argv[2], &maxCount, false))
        return STATUS_ERROR;
    if(!dbgsettracecondition(argv[1], maxCount))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    HistoryClear();
    if(stepOver)
        StepOver(callBack);
    else
        StepInto(callBack);
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugTraceIntoConditional(int argc, char* argv[])
{
    return cbDebugConditionalTrace((void*)cbTICNDStep, false, argc, argv);
}

CMDRESULT cbDebugTraceOverConditional(int argc, char* argv[])
{
    return cbDebugConditionalTrace((void*)cbTOCNDStep, true, argc, argv);
}

CMDRESULT cbDebugTraceIntoBeyondTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tibt", "0" };
        return cbDebugConditionalTrace((void*)cbTIBTStep, false, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTIBTStep, false, argc, argv);
}

CMDRESULT cbDebugTraceOverBeyondTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tobt", "0" };
        return cbDebugConditionalTrace((void*)cbTOBTStep, true, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTOBTStep, true, argc, argv);
}

CMDRESULT cbDebugTraceIntoIntoTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tiit", "0" };
        return cbDebugConditionalTrace((void*)cbTIITStep, false, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTIITStep, false, argc, argv);
}

CMDRESULT cbDebugTraceOverIntoTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "toit", "0" };
        return cbDebugConditionalTrace((void*)cbTOITStep, true, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTOITStep, true, argc, argv);
}

CMDRESULT cbDebugRunToParty(int argc, char* argv[])
{
    HistoryClear();
    EXCLUSIVE_ACQUIRE(LockRunToUserCode);
    std::vector<MODINFO> AllModules;
    ModGetList(AllModules);
    if(!RunToUserCodeBreakpoints.empty())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Run to party is busy.\n"));
        return STATUS_ERROR;
    }
    int party = atoi(argv[1]); // party is a signed integer
    for(auto i : AllModules)
    {
        if(i.party == party)
        {
            for(auto j : i.sections)
            {
                BREAKPOINT bp;
                if(!BpGet(j.addr, BPMEMORY, nullptr, &bp))
                {
                    RunToUserCodeBreakpoints.push_back(std::make_pair(j.addr, j.size));
                    SetMemoryBPXEx(j.addr, j.size, UE_MEMORY_EXECUTE, false, (void*)cbRunToUserCodeBreakpoint);
                }
            }
        }
    }
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugRunToUserCode(int argc, char* argv[])
{
    char* newargv[] = { "RunToParty", "0" };
    return cbDebugRunToParty(argc, newargv);
}
#include "cmd-tracing.h"
#include "debugger.h"
#include "historycontext.h"
#include "threading.h"
#include "module.h"
#include "console.h"
#include "cmd-debug-control.h"
#include "value.h"
#include "variable.h"
#include "TraceRecord.h"

extern std::vector<std::pair<duint, duint>> RunToUserCodeBreakpoints;

static bool cbDebugConditionalTrace(void(*callback)(), bool stepOver, int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    if(dbgtraceactive())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Trace already active"));
        return false;
    }
    duint maxCount;
    if(!BridgeSettingGetUint("Engine", "MaxTraceCount", &maxCount) || !maxCount)
        maxCount = 50000;
    if(argc > 2 && !valfromstring(argv[2], &maxCount, false))
        return false;
    if(!dbgsettracecondition(*argv[1] ? argv[1] : "0", maxCount))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression \"%s\"\n"), argv[1]);
        return false;
    }
    HistoryClear();

    if(stepOver)
        StepOverWrapper((void*)callback);
    else
        StepIntoWow64((void*)callback);
    return cbDebugRunInternal(1, argv);
}

bool cbDebugTraceIntoConditional(int argc, char* argv[])
{
    return cbDebugConditionalTrace(cbTraceIntoConditionalStep, false, argc, argv);
}

bool cbDebugTraceOverConditional(int argc, char* argv[])
{
    return cbDebugConditionalTrace(cbTraceOverConditionalStep, true, argc, argv);
}

bool cbDebugTraceIntoBeyondTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        const char* new_argv[] = { "tibt", "0" };
        return cbDebugConditionalTrace(cbTraceIntoBeyondTraceRecordStep, false, 2, (char**)new_argv);
    }
    else
        return cbDebugConditionalTrace(cbTraceIntoBeyondTraceRecordStep, false, argc, argv);
}

bool cbDebugTraceOverBeyondTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        const char* new_argv[] = { "tobt", "0" };
        return cbDebugConditionalTrace(cbTraceOverBeyondTraceRecordStep, true, 2, (char**)new_argv);
    }
    else
        return cbDebugConditionalTrace(cbTraceOverBeyondTraceRecordStep, true, argc, argv);
}

bool cbDebugTraceIntoIntoTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        const char* new_argv[] = { "tiit", "0" };
        return cbDebugConditionalTrace(cbTraceIntoIntoTraceRecordStep, false, 2, (char**)new_argv);
    }
    else
        return cbDebugConditionalTrace(cbTraceIntoIntoTraceRecordStep, false, argc, argv);
}

bool cbDebugTraceOverIntoTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        const char* new_argv[] = { "toit", "0" };
        return cbDebugConditionalTrace(cbTraceOverIntoTraceRecordStep, true, 2, (char**)new_argv);
    }
    else
        return cbDebugConditionalTrace(cbTraceOverIntoTraceRecordStep, true, argc, argv);
}

bool cbDebugRunToParty(int argc, char* argv[])
{
    HistoryClear();
    EXCLUSIVE_ACQUIRE(LockRunToUserCode);
    if(!RunToUserCodeBreakpoints.empty())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Run to party is busy.\n"));
        return false;
    }
    if(IsArgumentsLessThan(argc, 2))
        return false;
    int party = atoi(argv[1]); // party is a signed integer
    ModEnum([party](const MODINFO & i)
    {
        if(i.party == party)
        {
            for(auto j : i.sections)
            {
                BREAKPOINT bp;
                if(!BpGet(j.addr, BPMEMORY, nullptr, &bp))
                {
                    size_t size = DbgMemGetPageSize(j.addr);
                    RunToUserCodeBreakpoints.push_back(std::make_pair(j.addr, size));
                    SetMemoryBPXEx(j.addr, size, UE_MEMORY_EXECUTE, false, (void*)cbRunToUserCodeBreakpoint);
                }
            }
        }
    });
    return cbDebugRunInternal(1, argv);
}

bool cbDebugRunToUserCode(int argc, char* argv[])
{
    const char* newargv[] = { "RunToParty", "0" };
    return cbDebugRunToParty(2, (char**)newargv);
}

bool cbDebugTraceSetLog(int argc, char* argv[])
{
    auto text = argc > 1 ? argv[1] : "";
    auto condition = argc > 2 ? argv[2] : "";
    if(!dbgsettracelog(condition, text))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression \"%s\"\n"), condition);
        return false;
    }
    return true;
}

bool cbDebugTraceSetCommand(int argc, char* argv[])
{
    auto text = argc > 1 ? argv[1] : "";
    auto condition = argc > 2 ? argv[2] : "";
    if(!dbgsettracecmd(condition, text))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression \"%s\"\n"), condition);
        return false;
    }
    return true;
}

bool cbDebugTraceSetSwitchCondition(int argc, char* argv[])
{
    auto condition = argc > 1 ? argv[1] : "";
    if(!dbgsettraceswitchcondition(condition))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid expression \"%s\"\n"), condition);
        return false;
    }
    return true;
}

bool cbDebugTraceSetLogFile(int argc, char* argv[])
{
    auto fileName = argc > 1 ? argv[1] : "";
    return dbgsettracelogfile(fileName);
}

bool cbDebugStartTraceRecording(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    return TraceRecord.enableTraceRecording(true, argv[1]);
}

bool cbDebugStopTraceRecording(int argc, char* argv[])
{
    return TraceRecord.enableTraceRecording(false, nullptr);
}
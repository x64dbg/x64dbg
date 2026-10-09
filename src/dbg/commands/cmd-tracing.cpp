#include "cmd-tracing.h"
#include "debugger.h"
#include "threading.h"
#include "module.h"
#include "console.h"
#include "cmd-debug-control.h"
#include "value.h"
#include "variable.h"
#include "TraceRecord.h"

extern std::vector<std::pair<duint, duint>> RunToUserCodeBreakpoints;

static bool genericConditionalTraceCommand(TITANCBSTEP callback, STEPFUNCTION stepFunction, int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    if(dbgtraceactive())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Trace already active"));
        return false;
    }
    if(dbgisrunning())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Cannot start a trace when running, pause execution first."));
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

    stepFunction(callback);
    return cbDebugRunInternal(1, argv, history_clear);
}

static bool conditionalTraceIntoCommand(TITANCBSTEP callback, int argc, char* argv[])
{
    // Select step function based on party filter
    STEPFUNCTION stepFunction;
    auto party = dbggettracepartyfilter();
    if(party == mod_user)
        stepFunction = StepIntoUser;
    else if(party == mod_system)
        stepFunction = StepIntoSystem;
    else if(party == -1)
        stepFunction = StepIntoWrapper;
    else
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Unsupported party filter: %d\n"), party);
        return false;
    }
    return genericConditionalTraceCommand(callback, stepFunction, argc, argv);
}

static bool conditionalTraceOverCommand(TITANCBSTEP callback, int argc, char* argv[])
{
    // Select step function based on party filter
    STEPFUNCTION stepFunction;
    auto party = dbggettracepartyfilter();
    if(party == mod_user)
        stepFunction = StepOverUser;
    else if(party == mod_system)
        stepFunction = StepOverSystem;
    else if(party == -1)
        stepFunction = StepOverWrapper;
    else
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Unsupported party filter: %d\n"), party);
        return false;
    }
    return genericConditionalTraceCommand(callback, stepFunction, argc, argv);
}

bool cbDebugTraceIntoConditional(int argc, char* argv[])
{
    return conditionalTraceIntoCommand(cbTraceIntoConditionalStep, argc, argv);
}

bool cbDebugTraceOverConditional(int argc, char* argv[])
{
    return conditionalTraceOverCommand(cbTraceOverConditionalStep, argc, argv);
}

bool cbDebugTraceIntoBeyondTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        const char* new_argv[] = { "tibt", "0" };
        return conditionalTraceIntoCommand(cbTraceIntoBeyondTraceRecordStep, 2, (char**)new_argv);
    }
    else
        return conditionalTraceIntoCommand(cbTraceIntoBeyondTraceRecordStep, argc, argv);
}

bool cbDebugTraceOverBeyondTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        const char* new_argv[] = { "tobt", "0" };
        return conditionalTraceOverCommand(cbTraceOverBeyondTraceRecordStep, 2, (char**)new_argv);
    }
    else
        return conditionalTraceOverCommand(cbTraceOverBeyondTraceRecordStep, argc, argv);
}

bool cbDebugTraceIntoIntoTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        const char* new_argv[] = { "tiit", "0" };
        return conditionalTraceIntoCommand(cbTraceIntoIntoTraceRecordStep, 2, (char**)new_argv);
    }
    else
        return conditionalTraceIntoCommand(cbTraceIntoIntoTraceRecordStep, argc, argv);
}

bool cbDebugTraceOverIntoTraceRecord(int argc, char* argv[])
{
    if(argc == 1)
    {
        const char* new_argv[] = { "toit", "0" };
        return conditionalTraceOverCommand(cbTraceOverIntoTraceRecordStep, 2, (char**)new_argv);
    }
    else
        return conditionalTraceOverCommand(cbTraceOverIntoTraceRecordStep, argc, argv);
}

bool cbDebugRunToParty(int argc, char* argv[])
{
    if(dbgisrunning())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Cannot start a trace when running, pause execution first."));
        return false;
    }
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
                    RunToUserCodeBreakpoints.emplace_back(j.addr, size);
                    SetMemoryBPXEx(j.addr, size, UE_MEMORY_EXECUTE, false, cbRunToUserCodeBreakpoint);
                }
            }
        }
    });
    return cbDebugRunInternal(1, argv, history_clear);
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

bool cbDebugTraceSetLogFile(int argc, char* argv[])
{
    auto fileName = argc > 1 ? argv[1] : "";
    return dbgsettracelogfile(fileName);
}

bool cbDebugTraceSetStepFilter(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    auto filter = argv[1];
    if(_stricmp(filter, "none") == 0)
    {
        dbgsettracepartyfilter(-1);
        dputs(QT_TRANSLATE_NOOP("DBG", "Step filter set to: none"));
    }
    else if(_stricmp(filter, "user") == 0)
    {
        dbgsettracepartyfilter(mod_user);
        dputs(QT_TRANSLATE_NOOP("DBG", "Step filter set to: user"));
    }
    else if(_stricmp(filter, "system") == 0)
    {
        dbgsettracepartyfilter(mod_system);
        dputs(QT_TRANSLATE_NOOP("DBG", "Step filter set to: system"));
    }
    else
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid step filter \"%s\", valid options are: none, user, system\n"), filter);
        return false;
    }
    return true;
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
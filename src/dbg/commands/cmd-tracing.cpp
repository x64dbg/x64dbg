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
    return genericConditionalTraceCommand(callback, StepIntoWow64, argc, argv);
}

static bool conditionalTraceOverCommand(TITANCBSTEP callback, int argc, char* argv[])
{
    return genericConditionalTraceCommand(callback, StepOverWrapper, argc, argv);
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

bool cbDebugTraceWithModules(int argc, char* argv[])
{
   return conditionalTraceIntoCommand(cbTraceWithModulesStep, argc, argv);
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

bool cbAddTraceExcludedModule(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    if(TraceRecord.addExcludedModule(argv[1]))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Added module \"%s\" to trace excluded modules list\n"), argv[1]);
        if(strcmp(argv[1], "*") == 0)
            dputs(QT_TRANSLATE_NOOP("DBG", "The wildcard '*' will exclude all modules except those explicitly included"));
        return true;
    }

    dputs(QT_TRANSLATE_NOOP("DBG", "Failed to add module to trace excluded modules list"));
    return false;
}

bool cbDelTraceExcludedModule(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    if(TraceRecord.removeExcludedModule(argv[1]))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Removed module \"%s\" from trace excluded modules list\n"), argv[1]);
        return true;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "Module \"%s\" not found in trace excluded modules list\n"), argv[1]);
    return false;
}

bool cbListTraceExcludedModules(int argc, char* argv[])
{
    auto modules = TraceRecord.getExcludedModules();

    if(modules.empty())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No excluded modules for tracing"));
        return true;
    }

    dputs(QT_TRANSLATE_NOOP("DBG", "Trace excluded modules:"));
    for(const auto & module : modules)
        dprintf(QT_TRANSLATE_NOOP("DBG", "%s\n"), module.c_str());

    return true;
}

bool cbClearTraceExcludedModules(int argc, char* argv[])
{
    TraceRecord.clearExcludedModules();
    dputs(QT_TRANSLATE_NOOP("DBG", "Trace excluded modules list cleared"));
    return true;
}

bool cbAddTraceIncludedModule(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    if(TraceRecord.addIncludedModule(argv[1]))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Added module \"%s\" to trace included modules list\n"), argv[1]);
        if(strcmp(argv[1], "*") == 0)
            dputs(QT_TRANSLATE_NOOP("DBG", "The wildcard '*' will include all modules except those explicitly excluded"));
        return true;
    }

    dputs(QT_TRANSLATE_NOOP("DBG", "Failed to add module to trace included modules list"));
    return false;
}

bool cbDelTraceIncludedModule(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    if(TraceRecord.removeIncludedModule(argv[1]))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Removed module \"%s\" from trace included modules list\n"), argv[1]);
        return true;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "Module \"%s\" not found in trace included modules list\n"), argv[1]);
    return false;
}

bool cbListTraceIncludedModules(int argc, char* argv[])
{
    auto modules = TraceRecord.getIncludedModules();

    if(modules.empty())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No included modules for tracing"));
        return true;
    }

    dputs(QT_TRANSLATE_NOOP("DBG", "Trace included modules:"));
    for(const auto & module : modules)
        dprintf(QT_TRANSLATE_NOOP("DBG", "%s\n"), module.c_str());

    return true;
}

bool cbClearTraceIncludedModules(int argc, char* argv[])
{
    TraceRecord.clearIncludedModules();
    dputs(QT_TRANSLATE_NOOP("DBG", "Trace included modules list cleared"));
    return true;
}
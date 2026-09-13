#include "cmd-tracing.h"
#include "debugger.h"
#include "threading.h"
#include "module.h"
#include "console.h"
#include "cmd-debug-control.h"
#include "value.h"
#include "variable.h"
#include "TraceRecord.h"

#include "runtoparty.h"
#include <cerrno>
#include <climits>

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

    // Recording can already have queued an instruction before this filter was
    // selected. Remove an excluded pending entry before even the first step.
    TraceRecord.FilterPendingTraceRecord(dbggettracepartyfilter());
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
        stepFunction = StepIntoWow64;
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Cannot run to a module party when running, pause execution first."));
        return false;
    }
    if(IsArgumentsLessThan(argc, 2))
        return false;
    char* end;
    errno = 0;
    auto party = strtol(argv[1], &end, 10); // party is a signed decimal integer
    if(end == argv[1] || *end || errno == ERANGE || party < INT_MIN || party > INT_MAX)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid module party number."));
        return false;
    }
    if(!RunToParty(int(party), cbRunToPartyFinished))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Cannot run to party: busy, no executable target pages, or memory breakpoint conflict/setup failure."));
        return false;
    }
    TraceRecord.FlushTraceExecuteRecord();
    if(!cbDebugRunInternal(1, argv, history_clear))
    {
        RunToPartyClear();
        return false;
    }
    return true;
}

bool cbDebugRunToUserCode(int argc, char* argv[])
{
    const char* newargv[] = { "RunToParty", "0" };
    return cbDebugRunToParty(2, (char**)newargv);
}

bool cbDebugRunToSystemCode(int argc, char* argv[])
{
    const char* newargv[] = { "RunToParty", "1" };
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

    if(dbgtraceactive())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Cannot change the module filter during a trace."));
        return false;
    }
    auto mode = argc > 2 ? argv[2] : "step";
    bool runToParty = _stricmp(mode, "run") == 0;
    if(!runToParty && _stricmp(mode, "step") != 0)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid filter mode, valid options are: step, run"));
        return false;
    }
    auto filter = argv[1];
    if(_stricmp(filter, "none") == 0)
    {
        dbgsettracepartyfilter(-1);
        dputs(QT_TRANSLATE_NOOP("DBG", "Step filter set to: none"));
    }
    else if(_stricmp(filter, "user") == 0)
    {
        dbgsettracepartyfilter(mod_user, runToParty);
        dputs(QT_TRANSLATE_NOOP("DBG", "Step filter set to: user"));
    }
    else if(_stricmp(filter, "system") == 0)
    {
        dbgsettracepartyfilter(mod_system, runToParty);
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
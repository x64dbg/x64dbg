#include "cmd-conditional-breakpoint-control.h"
#include "breakpoint.h"
#include "debugger.h"
#include "console.h"
#include "variable.h"
#include "value.h"
#include <functional>

static bool cbDebugSetBPXTextCommon(BP_TYPE Type, int argc, char* argv[], const String & description, const std::function<bool(duint, BP_TYPE, const char*)> & setFunction)
{
    BREAKPOINT bp;
    if(IsArgumentsLessThan(argc, 2))
        return false;
    const char* value = "";
    if(argc > 2)
        value = argv[2];

    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!setFunction(bp.addr, Type, value))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set %s on breakpoint \"%s\"\n"), description.c_str(), argv[1]);
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    return true;
}

static bool cbDebugSetBPXNameCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "breakpoint name"))), BpSetName);
}

static bool cbDebugSetBPXConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "break condition"))), BpSetBreakCondition);
}

static bool cbDebugSetBPXLogCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "logging text"))), BpSetLogText);
}

static bool cbDebugSetBPXLogConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "logging condition"))), BpSetLogCondition);
}

static bool cbDebugSetBPXCommandCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "command on hit"))), BpSetCommandText);
}

static bool cbDebugSetBPXCommandConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "command condition"))), BpSetCommandCondition);
}

static bool cbDebugSetBPXLogFileCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "log file"))), BpSetLogFile);
}

static bool cbDebugSetBPXFastResumeCommon(BP_TYPE Type, int argc, char* argv[])
{
    BREAKPOINT bp;
    if(IsArgumentsLessThan(argc, 2))
        return false;
    auto fastResume = true;
    if(argc > 2)
    {
        duint value;
        if(!valfromstring(argv[2], &value, false))
            return false;
        fastResume = value != 0;
    }
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!BpSetFastResume(bp.addr, Type, fastResume))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set fast resume on breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    return true;
}

static bool cbDebugSetBPXSingleshootCommon(BP_TYPE Type, int argc, char* argv[])
{
    BREAKPOINT bp;
    if(IsArgumentsLessThan(argc, 2))
        return false;
    auto singleshoot = true;
    if(argc > 2)
    {
        duint value;
        if(!valfromstring(argv[2], &value, false))
            return false;
        singleshoot = value != 0;
    }
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!BpSetSingleshoot(bp.addr, Type, singleshoot))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set singleshoot on breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    return true;
}

static bool cbDebugSetBPXSilentCommon(BP_TYPE Type, int argc, char* argv[])
{
    BREAKPOINT bp;
    if(IsArgumentsLessThan(argc, 2))
        return false;
    auto silent = true;
    if(argc > 2)
    {
        duint value;
        if(!valfromstring(argv[2], &value, false))
            return false;
        silent = value != 0;
    }
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!BpSetSilent(bp.addr, Type, silent))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set silent on breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    return true;
}

static bool cbDebugGetBPXHitCountCommon(BP_TYPE Type, int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    BREAKPOINT bp;
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    varset("$result", bp.hitcount, false);
    return true;

}

static bool cbDebugResetBPXHitCountCommon(BP_TYPE Type, int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint value = 0;
    if(argc > 2)
        if(!valfromstring(argv[2], &value, false))
            return false;
    BREAKPOINT bp;
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!BpResetHitCount(bp.addr, Type, (uint32)value))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set hit count on breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    return true;
}

bool cbDebugSetBPXName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXLogFile(int argc, char* argv[])
{
    return cbDebugSetBPXLogFileCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXSingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXSilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPNORMAL, argc, argv);
}

bool cbDebugGetBPXHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPNORMAL, argc, argv);
}

bool cbDebugResetBPXHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPNORMAL, argc, argv);
}

bool cbDebugSetBPXHardwareName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareLogFile(int argc, char* argv[])
{
    return cbDebugSetBPXLogFileCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareSingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXHardwareSilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPHARDWARE, argc, argv);
}

bool cbDebugGetBPXHardwareHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPHARDWARE, argc, argv);
}

bool cbDebugResetBPXHardwareHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPHARDWARE, argc, argv);
}

bool cbDebugSetBPXMemoryName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemoryCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemoryLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemoryLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemoryCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemoryCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemoryLogFile(int argc, char* argv[])
{
    return cbDebugSetBPXLogFileCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemoryFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemorySingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXMemorySilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPMEMORY, argc, argv);
}

bool cbDebugGetBPXMemoryHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPMEMORY, argc, argv);
}

bool cbDebugResetBPXMemoryHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPMEMORY, argc, argv);
}

bool cbDebugSetBPXDLLName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLLogFile(int argc, char* argv[])
{
    return cbDebugSetBPXLogFileCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLSingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXDLLSilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPDLL, argc, argv);
}

bool cbDebugGetBPXDLLHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPDLL, argc, argv);
}

bool cbDebugResetBPXDLLHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPDLL, argc, argv);
}

bool cbDebugSetBPXExceptionName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionLogFile(int argc, char* argv[])
{
    return cbDebugSetBPXLogFileCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionSingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugSetBPXExceptionSilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugGetBPXExceptionHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPEXCEPTION, argc, argv);
}

bool cbDebugResetBPXExceptionHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPEXCEPTION, argc, argv);
}
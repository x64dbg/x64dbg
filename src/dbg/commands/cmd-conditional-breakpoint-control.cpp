#include "cmd-conditional-breakpoint-control.h"
#include "breakpoint.h"
#include "debugger.h"
#include "console.h"
#include "variable.h"
#include "value.h"

static CMDRESULT cbDebugSetBPXTextCommon(BP_TYPE Type, int argc, char* argv[], const String & description, const std::function<bool(duint, BP_TYPE, const char*)> & setFunction)
{
    BREAKPOINT bp;
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    char* value = "";
    if(argc > 2)
        value = argv[2];

    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    if(!setFunction(bp.addr, Type, value))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set %s on breakpoint \"%s\"\n"), description.c_str(), argv[1]);
        return STATUS_ERROR;
    }
    DebugUpdateBreakpointsViewAsync();
    return STATUS_CONTINUE;
}

static CMDRESULT cbDebugSetBPXNameCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "breakpoint name"))), BpSetName);
}

static CMDRESULT cbDebugSetBPXConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "break condition"))), BpSetBreakCondition);
}

static CMDRESULT cbDebugSetBPXLogCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "logging text"))), BpSetLogText);
}

static CMDRESULT cbDebugSetBPXLogConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "logging condition"))), BpSetLogCondition);
}

static CMDRESULT cbDebugSetBPXCommandCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "command on hit"))), BpSetCommandText);
}

static CMDRESULT cbDebugSetBPXCommandConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, String(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "command condition"))), BpSetCommandCondition);
}

static CMDRESULT cbDebugSetBPXFastResumeCommon(BP_TYPE Type, int argc, char* argv[])
{
    BREAKPOINT bp;
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!\n"));
        return STATUS_ERROR;
    }
    auto fastResume = true;
    if(argc > 2)
    {
        duint value;
        if(!valfromstring(argv[2], &value, false))
            return STATUS_ERROR;
        fastResume = value != 0;
    }
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    if(!BpSetFastResume(bp.addr, Type, fastResume))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set fast resume on breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    DebugUpdateBreakpointsViewAsync();
    return STATUS_CONTINUE;
}

static CMDRESULT cbDebugSetBPXSingleshootCommon(BP_TYPE Type, int argc, char* argv[])
{
    BREAKPOINT bp;
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!\n"));
        return STATUS_ERROR;
    }
    auto singleshoot = true;
    if(argc > 2)
    {
        duint value;
        if(!valfromstring(argv[2], &value, false))
            return STATUS_ERROR;
        singleshoot = value != 0;
    }
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    if(!BpSetSingleshoot(bp.addr, Type, singleshoot))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set singleshoot on breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    DebugUpdateBreakpointsViewAsync();
    return STATUS_CONTINUE;
}

static CMDRESULT cbDebugSetBPXSilentCommon(BP_TYPE Type, int argc, char* argv[])
{
    BREAKPOINT bp;
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!\n"));
        return STATUS_ERROR;
    }
    auto silent = true;
    if(argc > 2)
    {
        duint value;
        if(!valfromstring(argv[2], &value, false))
            return STATUS_ERROR;
        silent = value != 0;
    }
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    if(!BpSetSilent(bp.addr, Type, silent))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set silent on breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    DebugUpdateBreakpointsViewAsync();
    return STATUS_CONTINUE;
}

static CMDRESULT cbDebugGetBPXHitCountCommon(BP_TYPE Type, int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!\n"));
        return STATUS_ERROR;
    }
    BREAKPOINT bp;
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    varset("$result", bp.hitcount, false);
    return STATUS_CONTINUE;

}

static CMDRESULT cbDebugResetBPXHitCountCommon(BP_TYPE Type, int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!\n"));
        return STATUS_ERROR;
    }
    duint value = 0;
    if(argc > 2)
        if(!valfromstring(argv[2], &value, false))
            return STATUS_ERROR;
    BREAKPOINT bp;
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    if(!BpResetHitCount(bp.addr, Type, (uint32)value))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Can't set hit count on breakpoint \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    DebugUpdateBreakpointsViewAsync();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetBPXName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXSingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXSilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugGetBPXHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugResetBPXHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareSingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareSilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugGetBPXHardwareHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugResetBPXHardwareHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemorySingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemorySilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugGetBPXMemoryHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugResetBPXMemoryHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLSingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXDLLSilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugGetBPXDLLHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugResetBPXDLLHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPDLL, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionSingleshoot(int argc, char* argv[])
{
    return cbDebugSetBPXSingleshootCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugSetBPXExceptionSilent(int argc, char* argv[])
{
    return cbDebugSetBPXSilentCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugGetBPXExceptionHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPEXCEPTION, argc, argv);
}

CMDRESULT cbDebugResetBPXExceptionHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPEXCEPTION, argc, argv);
}
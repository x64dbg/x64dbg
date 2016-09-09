#include "_scriptapi_debug.h"

SCRIPT_EXPORT void Script::Debug::Wait()
{
    _plugin_waituntilpaused();
}

SCRIPT_EXPORT void Script::Debug::Run()
{
    if(DbgCmdExecDirect("run"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::Pause()
{
    if(DbgCmdExecDirect("pause"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::Stop()
{
    if(DbgCmdExecDirect("StopDebug"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepIn()
{
    if(DbgCmdExecDirect("StepInto"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepOver()
{
    if(DbgCmdExecDirect("StepOver"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepOut()
{
    if(DbgCmdExecDirect("StepOut"))
        Wait();
}

SCRIPT_EXPORT bool Script::Debug::SetBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bp %p", address);
    return DbgCmdExecDirect(command);
}

SCRIPT_EXPORT bool Script::Debug::DeleteBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bc %p", address);
    return DbgCmdExecDirect(command);

}
SCRIPT_EXPORT bool Script::Debug::DisableBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bd %p", address);
    return DbgCmdExecDirect(command);
}

SCRIPT_EXPORT bool Script::Debug::SetHardwareBreakpoint(duint address, HardwareType type)
{
    char command[128] = "";
    const char* types[] = { "rw", "w", "x" };
    sprintf_s(command, "bphws %p, %s", address, types[type]);
    return DbgCmdExecDirect(command);
}

SCRIPT_EXPORT bool Script::Debug::DeleteHardwareBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bphwc %p", address);
    return DbgCmdExecDirect(command);
}
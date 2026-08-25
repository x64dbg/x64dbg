#include "_scriptapi_debug.h"

SCRIPT_EXPORT void Script::Debug::Wait()
{
    _plugin_waituntilpaused();
}

SCRIPT_EXPORT void Script::Debug::Run()
{
    if(DbgCmdExecAsyncDirect("run"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::Pause()
{
    if(DbgCmdExecAsyncDirect("pause"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::Stop()
{
    if(DbgCmdExecAsyncDirect("StopDebug"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepIn()
{
    if(DbgCmdExecAsyncDirect("StepInto"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepOver()
{
    if(DbgCmdExecAsyncDirect("StepOver"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepOut()
{
    if(DbgCmdExecAsyncDirect("StepOut"))
        Wait();
}

SCRIPT_EXPORT bool Script::Debug::SetBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bp %p", (void*)address);
    return DbgCmdExecAsyncDirect(command);
}

SCRIPT_EXPORT bool Script::Debug::DeleteBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bc %p", (void*)address);
    return DbgCmdExecAsyncDirect(command);

}
SCRIPT_EXPORT bool Script::Debug::DisableBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bd %p", (void*)address);
    return DbgCmdExecAsyncDirect(command);
}

SCRIPT_EXPORT bool Script::Debug::SetHardwareBreakpoint(duint address, HardwareType type)
{
    char command[128] = "";
    const char* types[] = { "rw", "w", "x" };
    sprintf_s(command, "bphws %p, %s", (void*)address, types[type]);
    return DbgCmdExecAsyncDirect(command);
}

SCRIPT_EXPORT bool Script::Debug::DeleteHardwareBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bphwc %p", (void*)address);
    return DbgCmdExecAsyncDirect(command);
}
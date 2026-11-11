#include "_scriptapi_debug.h"
#include "_dbgfunctions.h"

static bool refreshBreakpointViews(bool success)
{
    if(success)
        GuiUpdateAllViews();
    return success;
}

static bool setBreakpoint(duint address, BPXTYPE type, unsigned int typeEx)
{
    auto functions = DbgFunctions();
    BP_REF ref{};
    if(!functions->BpRefVa(&ref, type, address))
        return false;
    if(functions->BpRefExists(&ref))
        return functions->BpSetFieldNumber(&ref, bpf_enabled, 1);
    BP_CREATEINFO info{};
    info.size = sizeof(info);
    info.type = type;
    info.address = address;
    info.enabled = true;
    info.typeEx = typeEx;
    return functions->BpCreate(&info, &ref);
}

static bool removeBreakpoint(duint address, BPXTYPE type)
{
    BP_REF ref{};
    return DbgFunctions()->BpRefVa(&ref, type, address) && DbgFunctions()->BpRemove(&ref);
}

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
    return refreshBreakpointViews(setBreakpoint(address, bp_normal, 0));
}

SCRIPT_EXPORT bool Script::Debug::DeleteBreakpoint(duint address)
{
    return refreshBreakpointViews(removeBreakpoint(address, bp_normal));
}
SCRIPT_EXPORT bool Script::Debug::DisableBreakpoint(duint address)
{
    BP_REF ref{};
    return refreshBreakpointViews(DbgFunctions()->BpRefVa(&ref, bp_normal, address) &&
                                  DbgFunctions()->BpSetFieldNumber(&ref, bpf_enabled, 0));
}

SCRIPT_EXPORT bool Script::Debug::SetHardwareBreakpoint(duint address, HardwareType type)
{
    if(type < HardwareAccess || type > HardwareExecute)
        return false;
    return refreshBreakpointViews(setBreakpoint(address, bp_hardware, static_cast<unsigned int>(type)));
}

SCRIPT_EXPORT bool Script::Debug::DeleteHardwareBreakpoint(duint address)
{
    return refreshBreakpointViews(removeBreakpoint(address, bp_hardware));
}
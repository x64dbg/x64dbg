#include "_scriptapi_debug.h"

SCRIPT_EXPORT void Script::Debug::Wait()
{
    _plugin_waituntilpaused();
}

SCRIPT_EXPORT void Script::Debug::Run()
{
    DbgCmdExecDirect("run");
    Wait();
}

SCRIPT_EXPORT void Script::Debug::Pause()
{
    DbgCmdExecDirect("pause");
    Wait();
}

SCRIPT_EXPORT void Script::Debug::Stop()
{
    DbgCmdExecDirect("StopDebug");
    Wait();
}

SCRIPT_EXPORT void Script::Debug::StepIn()
{
    DbgCmdExecDirect("StepInto");
    Wait();
}

SCRIPT_EXPORT void Script::Debug::StepOver()
{
    DbgCmdExecDirect("StepOver");
    Wait();
}

SCRIPT_EXPORT void Script::Debug::StepOut()
{
    DbgCmdExecDirect("StepOut");
    Wait();
}
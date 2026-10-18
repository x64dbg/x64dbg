#include <Windows.h>

#include <cstring>

#include "_plugins.h"
#include "_dbgfunctions.h"
#include "bridgelist.h"

namespace
{
    bool cbHandlesAssert(int, char**)
    {
        const auto address = DbgValFromString("handles_enum:HighHandle");
        duint expected = 0;
        if(!_plugin_testassert(address && DbgMemRead(address, &expected, sizeof(expected)), "read HighHandle"))
            return false;
        if(!_plugin_testassert(expected > 0xFFFF, "target handle exceeds 16 bits: %p", (void*)expected))
            return false;

        BridgeList<HANDLEINFO> handles;
        if(!_plugin_testassert(DbgFunctions()->EnumHandles(&handles), "EnumHandles succeeds"))
            return false;
        _plugin_logprintf("[x64dbg-test] PID=%lu handles=%d expected=%p\n", DbgGetProcessId(), handles.Count(), (void*)expected);
        for(int i = 0; i < handles.Count(); i++)
        {
            if(handles[i].Handle != expected)
                continue;
            HANDLE local = nullptr;
            if(!_plugin_testassert(DuplicateHandle(DbgGetProcessHandle(), (HANDLE)handles[i].Handle, GetCurrentProcess(), &local, 0, FALSE, DUPLICATE_SAME_ACCESS), "enumerated handle can be duplicated"))
                return false;
            DWORD flags = 0;
            const auto valid = GetHandleInformation(local, &flags);
            CloseHandle(local);
            return _plugin_testassert(valid != FALSE, "full-width handle is valid");
        }
        return _plugin_testassert(false, "EnumHandles must include the full-width target handle");
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    return _plugin_registercommand(initStruct->pluginHandle, "handlesenumassert", cbHandlesAssert, true);
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

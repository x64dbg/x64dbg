#include <Windows.h>

#include <cstring>

#include "_plugins.h"
#include "bridgemain.h"

namespace
{
    int gPluginHandle = 0;

    bool cbIssue3932Assert(int, char**)
    {
        DBGPROCESSINFO* entries = nullptr;
        int count = 0;
        if(!_plugin_testassert(DbgFunctions()->GetProcessList(&entries, &count), "GetProcessList failed"))
            return false;

        const auto pid = static_cast<DWORD>(DbgValFromString("$pid"));
        if(!_plugin_testassert(pid != 0, "$pid is 0"))
            return false;

        bool found = false;
        for(int i = 0; i < count; i++)
        {
            if(entries[i].dwProcessId != pid)
                continue;

            found = true;
            if(!_plugin_testassert(strcmp(entries[i].szExeArgs, "issue3932-payload") == 0,
                                   "szExeArgs is \"%s\", expected issue3932-payload", entries[i].szExeArgs))
                return false;
            break;
        }

        return _plugin_testassert(found, "debuggee pid %u was not in the process list", pid);
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercommand(gPluginHandle, "issue3932assert", cbIssue3932Assert, false);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

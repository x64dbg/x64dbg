#include <Windows.h>

#include <cstring>

#include "_plugins.h"
#include "_dbgfunctions.h"

namespace
{
    int gPluginHandle = 0;

    duint cbIsTracing(int, const duint*, void*)
    {
        return DbgFunctions()->IsTracing() ? 1 : 0;
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    return _plugin_registerexprfunction(gPluginHandle, "istracing", 0, cbIsTracing, nullptr);
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

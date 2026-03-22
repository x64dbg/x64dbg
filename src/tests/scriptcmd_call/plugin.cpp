#include <Windows.h>

#include <atomic>
#include <cstring>
#include <string>

#include "_plugins.h"
#include "bridgemain.h"

namespace
{
    int gPluginHandle = 0;
    std::atomic<unsigned int> gOuterDispatchCount{ 0 };
    std::atomic<unsigned int> gInnerDispatchCount{ 0 };

    bool pluginDirectModeEnabled()
    {
        return DbgValFromString("$plugin_direct_mode") != 0;
    }

    duint resolveAddress(const char* expression)
    {
        return DbgValFromString(expression);
    }

    bool dispatchScriptCallback(const char* label, std::atomic<unsigned int>& counter)
    {
        counter.fetch_add(1);
        const auto command = std::string("scriptcmd call ") + label;
        return _plugin_testassert(DbgCmdExecDirect(command.c_str()), "DbgCmdExecDirect failed for %s", label);
    }

    void cbPlugin(CBTYPE cbType, void* callbackInfo)
    {
        if(cbType == CB_INITDEBUG)
        {
            gOuterDispatchCount = 0;
            gInnerDispatchCount = 0;
            return;
        }

        if(cbType != CB_BREAKPOINT || !pluginDirectModeEnabled())
            return;

        const auto info = static_cast<PLUG_CB_BREAKPOINT*>(callbackInfo);
        if(info == nullptr || info->breakpoint == nullptr || info->breakpoint->type != bp_normal)
            return;

        const auto outerAddress = resolveAddress("scriptcmd_call:OuterHit");
        const auto innerAddress = resolveAddress("scriptcmd_call:InnerHit");
        if(info->breakpoint->addr == outerAddress)
            dispatchScriptCallback("onouter", gOuterDispatchCount);
        else if(info->breakpoint->addr == innerAddress)
            dispatchScriptCallback("oninner", gInnerDispatchCount);
    }

    bool cbPluginDirectAssert(int, char**)
    {
        if(!_plugin_testassert(gOuterDispatchCount.load() == 1, "expected exactly 1 plugin-direct outer dispatch, got %u", gOuterDispatchCount.load()))
            return false;
        return _plugin_testassert(gInnerDispatchCount.load() == 1, "expected exactly 1 plugin-direct inner dispatch, got %u", gInnerDispatchCount.load());
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), "ScriptCmdCallPlugin", _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercallback(gPluginHandle, CB_INITDEBUG, cbPlugin);
    _plugin_registercallback(gPluginHandle, CB_BREAKPOINT, cbPlugin);
    _plugin_registercommand(gPluginHandle, "plugindirectassert", cbPluginDirectAssert, false);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

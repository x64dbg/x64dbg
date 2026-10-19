#include <Windows.h>

#include <cstring>

#include "_plugins.h"
#include "bridgemain.h"
#include "_dbgfunctions.h"

namespace
{
    bool parseType(const char* text, BPXTYPE & type)
    {
        if(strcmp(text, "sw") == 0)
            type = bp_normal;
        else if(strcmp(text, "hw") == 0)
            type = bp_hardware;
        else if(strcmp(text, "mem") == 0)
            type = bp_memory;
        else if(strcmp(text, "dll") == 0)
            type = bp_dll;
        else if(strcmp(text, "ex") == 0)
            type = bp_exception;
        else
            return _plugin_testassert(false, "unknown breakpoint type: %s", text);
        return true;
    }

    bool findBreakpoint(BPXTYPE type, duint address, BRIDGEBP & result, const char* module = nullptr)
    {
        BPMAP list{};
        DbgGetBpList(type, &list);
        bool found = false;
        for(int i = 0; i < list.count; i++)
        {
            if(type == bp_dll ? module && _stricmp(list.bp[i].mod, module) == 0 : list.bp[i].addr == address)
            {
                result = list.bp[i];
                found = true;
                break;
            }
        }
        BridgeFree(list.bp);
        return found;
    }

    // Check the legacy list and reference APIs independently: querying bpf_active
    // can itself modify the stored active flag in the pre-refactor implementation.
    bool cbState(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 6, "usage: bprstate type, address, enabled, active, list|ref"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type))
            return false;
        const auto address = type == bp_dll ? 0 : DbgValFromString(argv[2]);
        if(!_plugin_testassert(type == bp_dll || address != 0, "invalid breakpoint address: %s", argv[2]))
            return false;
        const auto expectedEnabled = DbgValFromString(argv[3]);
        const auto expectedActive = DbgValFromString(argv[4]);
        duint enabled = 0, active = 0;
        if(strcmp(argv[5], "list") == 0)
        {
            BRIDGEBP bp{};
            if(!_plugin_testassert(findBreakpoint(type, address, bp, argv[2]), "breakpoint missing from DbgGetBpList: %s", argv[2]))
                return false;
            enabled = bp.enabled;
            active = bp.active;
        }
        else if(strcmp(argv[5], "ref") == 0)
        {
            auto functions = DbgFunctions();
            BP_REF ref{};
            if(type == bp_dll)
                functions->BpRefDll(&ref, argv[2]);
            else if(type == bp_exception)
                functions->BpRefException(&ref, static_cast<unsigned int>(address));
            else if(!_plugin_testassert(functions->BpRefVa(&ref, type, address), "could not create breakpoint reference"))
                return false;
            if(!_plugin_testassert(functions->BpRefExists(&ref),
                                   "breakpoint reference missing: %s", argv[2]))
                return false;
            if(!_plugin_testassert(functions->BpGetFieldNumber(&ref, bpf_enabled, &enabled) &&
                                   functions->BpGetFieldNumber(&ref, bpf_active, &active),
                                   "could not read breakpoint reference fields: %s", argv[2]))
                return false;
        }
        else
            return _plugin_testassert(false, "unknown breakpoint API: %s", argv[5]);

        if(!_plugin_testassert(enabled == expectedEnabled, "%s %s breakpoint enabled=%llu, expected %llu (%s)",
                               argv[5], argv[1], static_cast<unsigned long long>(enabled),
                               static_cast<unsigned long long>(expectedEnabled), argv[2]))
            return false;
        return _plugin_testassert(active == expectedActive, "%s %s breakpoint active=%llu, expected %llu (%s)",
                                  argv[5], argv[1], static_cast<unsigned long long>(active),
                                  static_cast<unsigned long long>(expectedActive), argv[2]);
    }

    bool cbDllHits(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprdllhits module, expected_hits"))
            return false;
        BRIDGEBP bp{};
        if(!_plugin_testassert(findBreakpoint(bp_dll, 0, bp, argv[1]), "DLL breakpoint missing: %s", argv[1]))
            return false;
        const auto expected = DbgValFromString(argv[2]);
        return _plugin_testassert(bp.hitCount == expected, "DLL breakpoint %s hit count=%llu, expected %llu", argv[1],
                                  static_cast<unsigned long long>(bp.hitCount), static_cast<unsigned long long>(expected));
    }

    bool cbCommand(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprcommand expected_success, command"))
            return false;
        const bool expected = DbgValFromString(argv[1]) != 0;
        const bool actual = DbgCmdExecDirect(argv[2]);
        return _plugin_testassert(actual == expected, "command '%s' returned %s, expected %s", argv[2],
                                  actual ? "success" : "failure", expected ? "success" : "failure");
    }

    bool cbAbsent(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprabsent type, address"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type))
            return false;
        const auto address = type == bp_dll ? 0 : DbgValFromString(argv[2]);
        if(!_plugin_testassert(type == bp_dll || address != 0, "invalid breakpoint address: %s", argv[2]))
            return false;
        BRIDGEBP bp{};
        return _plugin_testassert(!findBreakpoint(type, address, bp, argv[2]), "deleted %s breakpoint still exists: %s", argv[1], argv[2]);
    }
}

#include "removal_failure.h"
#include "installation_failure.h"
#include "dll_removal_failure.h"
#include "lifecycle_api.h"

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    _plugin_registercallback(initStruct->pluginHandle, CB_LOADDLL, cbDllRemoval);
    return _plugin_registercommand(initStruct->pluginHandle, "bprstate", cbState, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprdllhits", cbDllHits, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprcommand", cbCommand, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprabsent", cbAbsent, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprremoval", cbRemovalFailure, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprinstallation", cbInstallationFailure, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprdllarm", cbArmDllRemoval, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprdllcheck", cbCheckDllRemoval, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprapienable", cbApiEnable, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprapicreate", cbApiCreate, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprapifailure", cbApiFailure, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprapisingleshoot", cbApiSingleshoot, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprapireconfigure", cbApiReconfigureFailure, true) &&
           _plugin_registercommand(initStruct->pluginHandle, "bprapipending", cbApiPending, true);
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

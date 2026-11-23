#pragma once

#include <atomic>

namespace
{
    std::string gDllRemovalCommand;
    std::atomic<bool> gDllRemovalArmed{ false };
    std::atomic<unsigned int> gDllRemovalChecks{ 0 };

    bool cbArmDllRemoval(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 2, "usage: bprdllarm command"))
            return false;
        gDllRemovalCommand = argv[1];
        gDllRemovalChecks = 0;
        gDllRemovalArmed = true;
        return true;
    }

    void cbDllRemoval(CBTYPE, void* callbackInfo)
    {
        const auto info = static_cast<PLUG_CB_LOADDLL*>(callbackInfo);
        if(_stricmp(info->modname, "winmm.dll") != 0 || !gDllRemovalArmed.exchange(false))
            return;

        // The loader has consumed the one-shot DLL engine entry, but the
        // generic breakpoint handler has not yet deleted its database entry.
        // Reentrant removal therefore genuinely fails without any test hook.
        BRIDGEBP bp{};
        if(!_plugin_testassert(findBreakpoint(bp_dll, 0, bp, "winmm.dll") && bp.enabled && bp.active,
                               "one-shot DLL breakpoint missing before reentrant removal"))
            return;
        const bool result = DbgCmdExecDirect(gDllRemovalCommand.c_str());
        if(!_plugin_testassert(findBreakpoint(bp_dll, 0, bp, "winmm.dll"),
                               "failed DLL removal lost its pending database entry") ||
                !_plugin_testassert(bp.enabled && bp.active, "failed DLL removal cleared pending enabled/active state") ||
                !_plugin_testassert(!result, "failed DLL removal must report failure"))
            return;
        gDllRemovalChecks++;
    }

    bool cbCheckDllRemoval(int argc, char**)
    {
        if(!_plugin_testassert(argc == 1 && gDllRemovalChecks == 1 && !gDllRemovalArmed,
                               "expected one successful DLL removal-failure check"))
            return false;
        BRIDGEBP bp{};
        return _plugin_testassert(!findBreakpoint(bp_dll, 0, bp, "winmm.dll"),
                                  "generic handler did not clean up the consumed one-shot DLL breakpoint");
    }
}

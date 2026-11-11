#pragma once

#include "TitanEngine/TitanEngine.h"

namespace
{
    bool failHardwareInstallation(ULONG_PTR, DWORD, TitanHardwareBreakpointType, TitanHardwareBreakpointSize, TITANCBHWBP)
    {
        InterlockedIncrement(&gRemovalCalls);
        return false;
    }

    bool failMemoryInstallation(ULONG_PTR, SIZE_T, TitanMemoryBreakpointType, bool, TITANCBMEMBP)
    {
        InterlockedIncrement(&gRemovalCalls);
        return false;
    }

    bool cbInstallationFailure(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 4, "usage: bprinstallation type, address, command"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type) || !_plugin_testassert(type == bp_hardware || type == bp_memory,
                "unsupported installation failure type"))
            return false;
        const auto address = DbgValFromString(argv[2]);
        BRIDGEBP bp{};
        if(!_plugin_testassert(!findBreakpoint(type, address, bp), "installation test requires no existing breakpoint"))
            return false;
#ifdef _WIN64
        auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x64dbg.dll"));
#else
        auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x32dbg.dll"));
#endif
        bool result;
        {
            // Reuse the scoped import override: reject setup before the engine
            // installs anything, then restore the import before retrying.
            ScopedRemovalFailure failure;
            if(!_plugin_testassert(failure.installImport(base,
                                   type == bp_hardware ? "SetHardwareBreakPoint" : "SetMemoryBPXEx",
                                   type == bp_hardware ? reinterpret_cast<void*>(&failHardwareInstallation) :
                                   reinterpret_cast<void*>(&failMemoryInstallation)), "could not install scoped setup failure"))
                return false;
            result = DbgCmdExecDirect(argv[3]);
            if(!_plugin_testassert(failure.restore(), "could not restore installation import"))
                return false;
        }
        if(!_plugin_testassert(gRemovalCalls == 1, "expected one intercepted engine installation, got %ld", gRemovalCalls) ||
                !_plugin_testassert(!result, "failed engine installation must make command '%s' fail", argv[3]) ||
                !_plugin_testassert(!findBreakpoint(type, address, bp), "failed installation left a phantom breakpoint database entry"))
            return false;

        if(!_plugin_testassert(DbgCmdExecDirect(argv[3]), "retry of '%s' failed after restoring the engine", argv[3]) ||
                !_plugin_testassert(findBreakpoint(type, address, bp) && bp.enabled && bp.active,
                                    "successful installation did not set enabled/active state") ||
                !engineBreakpointInstalled(bp, true))
            return false;
        return true;
    }
}

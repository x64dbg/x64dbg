#pragma once

#include "_scriptapi_debug.h"

namespace
{
    bool cbDerivedMemory(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprderivedmemory size|deferred|hit|single, address"))
            return false;
        const auto address = DbgValFromString(argv[2]);
        const bool deferred = strcmp(argv[1], "deferred") == 0;
        const bool single = strcmp(argv[1], "single") == 0;
        const bool hit = strcmp(argv[1], "hit") == 0;
        if(!_plugin_testassert(deferred || single || hit || strcmp(argv[1], "size") == 0, "unknown derived-memory mode"))
            return false;
        auto info = apiCreateInfo(bp_memory, address, nullptr);
        info.memsize = 0;
        info.enabled = !deferred;
        info.singleshoot = single;
        BP_REF ref{};
        auto functions = DbgFunctions();
        if(!_plugin_testassert(functions->BpCreate(&info, &ref), "could not create derived-size memory breakpoint"))
            return false;
        if(deferred)
        {
            {
                ScopedRemovalFailure failure;
                if(!_plugin_testassert(apiRejectInstallation(failure, bp_memory), "could not reject derived-size installation") ||
                        !_plugin_testassert(!functions->BpSetFieldNumber(&ref, bpf_enabled, 1), "rejected derived-size enable must fail") ||
                        !apiState(ref, false, false) ||
                        !_plugin_testassert(failure.restore(), "could not restore derived-size installation"))
                    return false;
            }
            if(!_plugin_testassert(functions->BpSetFieldNumber(&ref, bpf_enabled, 1), "could not enable derived-size breakpoint"))
                return false;
        }
        if(single)
        {
            return _plugin_testassert(functions->BpSetFieldText(&ref, bpf_commandtext, "mov $derivedhit, 1"),
                                      "could not configure derived single-shot command");
        }
        if(hit)
        {
            return _plugin_testassert(functions->BpSetFieldText(&ref, bpf_breakcondition, "0") &&
                                      functions->BpSetFieldNumber(&ref, bpf_fastresume, 1), "could not configure derived breakpoint condition");
        }
        MEMORY_BASIC_INFORMATION region{};
        if(!_plugin_testassert(VirtualQueryEx(DbgGetProcessHandle(), reinterpret_cast<void*>(address), &region, sizeof(region)) == sizeof(region) &&
                               reinterpret_cast<duint>(region.BaseAddress) == address, "derived-size test requires a region base"))
            return false;
        const auto size = functions->MemBpSize(address);
        return _plugin_testassert(size == region.RegionSize && size != 0, "stored derived memory size=%llu, expected %llu",
                                  static_cast<unsigned long long>(size), static_cast<unsigned long long>(region.RegionSize)) &&
               _plugin_testassert(functions->MemBpSize(address + region.RegionSize - 1) == size,
                                  "derived memory breakpoint must cover its last byte");
    }

    bool cbDerivedMemoryHits(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 2, "usage: bprderivedhits address"))
            return false;
        BP_REF ref{};
        duint hits = 0;
        auto functions = DbgFunctions();
        return _plugin_testassert(functions->BpRefVa(&ref, bp_memory, DbgValFromString(argv[1])) &&
                                  functions->BpGetFieldNumber(&ref, bpf_hitcount, &hits) && hits == 1,
                                  "derived memory breakpoint hit count=%llu, expected 1", static_cast<unsigned long long>(hits));
    }

    using FullRefresh = void(*)();
    FullRefresh gOriginalFullRefresh = nullptr;
    DWORD gRefreshThread = 0;
    LONG gFullRefreshCalls = 0;

    void observeFullRefresh()
    {
        if(GetCurrentThreadId() == gRefreshThread)
            InterlockedIncrement(&gFullRefreshCalls);
        gOriginalFullRefresh();
    }

    // Observe and forward the debugger's regular bridge import. No GUI or engine
    // code is patched, and calls on unrelated background threads are not counted.
    class ScopedFullRefresh
    {
        void** mSlot = nullptr;
        DWORD mProtection = 0;

    public:
        ScopedFullRefresh() = default;
        ScopedFullRefresh(const ScopedFullRefresh &) = delete;
        ScopedFullRefresh & operator=(const ScopedFullRefresh &) = delete;

        ~ScopedFullRefresh()
        {
            _plugin_testassert(restore(), "could not restore full-refresh import");
        }

        bool install()
        {
#ifdef _WIN64
            auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x64dbg.dll"));
            const char* bridge = "x64bridge.dll";
#else
            auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x32dbg.dll"));
            const char* bridge = "x32bridge.dll";
#endif
            if(!base)
                return false;
            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
            if(!directory.VirtualAddress)
                return false;
            auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress);
            for(; descriptor->Name; descriptor++)
            {
                if(_stricmp(reinterpret_cast<const char*>(base + descriptor->Name), bridge) != 0 || !descriptor->OriginalFirstThunk)
                    continue;
                auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->OriginalFirstThunk);
                auto slots = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->FirstThunk);
                for(size_t i = 0; names[i].u1.AddressOfData; i++)
                {
                    if(IMAGE_SNAP_BY_ORDINAL(names[i].u1.Ordinal))
                        continue;
                    auto imported = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names[i].u1.AddressOfData);
                    if(strcmp(reinterpret_cast<const char*>(imported->Name), "GuiUpdateAllViews") != 0)
                        continue;
                    auto slot = reinterpret_cast<void**>(&slots[i].u1.Function);
                    if(!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &mProtection))
                        return false;
                    gRefreshThread = GetCurrentThreadId();
                    gFullRefreshCalls = 0;
                    gOriginalFullRefresh = reinterpret_cast<FullRefresh>(*slot);
                    InterlockedExchangePointer(slot, reinterpret_cast<void*>(&observeFullRefresh));
                    mSlot = slot;
                    DWORD unused;
                    return VirtualProtect(slot, sizeof(*slot), mProtection, &unused) != FALSE;
                }
            }
            return false;
        }

        bool restore()
        {
            if(!mSlot)
                return true;
            DWORD protection;
            if(!VirtualProtect(mSlot, sizeof(*mSlot), PAGE_READWRITE, &protection))
                return false;
            InterlockedExchangePointer(mSlot, reinterpret_cast<void*>(gOriginalFullRefresh));
            DWORD unused;
            if(!VirtualProtect(mSlot, sizeof(*mSlot), mProtection, &unused))
                return false;
            mSlot = nullptr;
            return true;
        }
    };

    class ScopedGuiUpdates
    {
        bool mWasDisabled = GuiIsUpdateDisabled();

    public:
        ScopedGuiUpdates()
        {
            GuiUpdateEnable(false);
        }

        ~ScopedGuiUpdates()
        {
            if(mWasDisabled)
                GuiUpdateDisable();
        }
    };

    LONG gRejectedDetachCalls = 0;

    bool rejectDetach(DWORD)
    {
        InterlockedIncrement(&gRejectedDetachCalls);
        return false;
    }

    bool cbFailedDetach(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprfaileddetach removal|detach, software_address"))
            return false;
        const bool removal = strcmp(argv[1], "removal") == 0;
        if(!_plugin_testassert(removal || strcmp(argv[1], "detach") == 0, "unknown detach-failure mode"))
            return false;
        const auto address = DbgValFromString(argv[2]);
        BRIDGEBP before{};
        if(!_plugin_testassert(findBreakpoint(bp_normal, address, before) && before.enabled && before.active,
                               "detach test requires an installed software breakpoint") || !engineBreakpointInstalled(before, true))
            return false;
#ifdef _WIN64
        auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x64dbg.dll"));
#else
        auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x32dbg.dll"));
#endif
        bool result;
        {
            // Never detach the test process, even if removal unexpectedly succeeds.
            ScopedRemovalFailure detachFailure;
            ScopedRemovalFailure removalFailure;
            if(!_plugin_testassert(detachFailure.installImport(base, "DetachDebuggerEx", reinterpret_cast<void*>(&rejectDetach)),
                                   "could not reject engine detach"))
                return false;
            if(removal && !_plugin_testassert(removalFailure.install(bp_normal), "could not reject pre-detach removal"))
                return false;
            gRejectedDetachCalls = 0;
            result = DbgCmdExecDirect("detach");
            if(!_plugin_testassert(removalFailure.restore() && detachFailure.restore(), "could not restore detach test imports"))
                return false;
        }
        if(!_plugin_testassert(!result && DbgIsDebugging(), "rejected detach must fail and retain the debug session") ||
                !_plugin_testassert(gRejectedDetachCalls == (removal ? 0 : 1) && gRemovalCalls == (removal ? 1 : 0),
                                    "unexpected pre-detach removal/detach calls: %ld/%ld", gRemovalCalls, gRejectedDetachCalls))
            return false;
        BRIDGEBP after{};
        return _plugin_testassert(findBreakpoint(bp_normal, address, after) && after.enabled && after.active,
                                  "failed detach did not retain/restore software breakpoint state") &&
               engineBreakpointInstalled(after, true);
    }

    bool cbScriptRefresh(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprscriptrefresh set|reenable|disable|delete|set-hw|delete-hw|missing, address"))
            return false;
        const auto address = DbgValFromString(argv[2]);
        const bool set = strcmp(argv[1], "set") == 0;
        const bool reenable = strcmp(argv[1], "reenable") == 0;
        const bool disable = strcmp(argv[1], "disable") == 0;
        const bool remove = strcmp(argv[1], "delete") == 0;
        const bool setHw = strcmp(argv[1], "set-hw") == 0;
        const bool removeHw = strcmp(argv[1], "delete-hw") == 0;
        const bool missing = strcmp(argv[1], "missing") == 0;
        ScopedGuiUpdates updates;
        if(!_plugin_testassert(set || reenable || disable || remove || setHw || removeHw || missing, "unknown script-refresh operation") ||
                !_plugin_testassert(!GuiIsUpdateDisabled(), "GUI refresh must not be disabled"))
            return false;
        auto functions = DbgFunctions();
        const auto type = setHw || removeHw ? bp_hardware : bp_normal;
        BP_REF ref{};
        if(reenable || disable || remove || removeHw)
        {
            auto info = apiCreateInfo(type, address, nullptr);
            info.enabled = !reenable;
            if(!_plugin_testassert(functions->BpCreate(&info, &ref), "could not prepare script-refresh breakpoint"))
                return false;
        }
        bool result;
        {
            ScopedFullRefresh observer;
            if(!_plugin_testassert(observer.install(), "could not observe full-view refresh import"))
                return false;
            if(set || reenable)
                result = Script::Debug::SetBreakpoint(address);
            else if(disable)
                result = Script::Debug::DisableBreakpoint(address);
            else if(remove || missing)
                result = Script::Debug::DeleteBreakpoint(address);
            else if(setHw)
                result = Script::Debug::SetHardwareBreakpoint(address);
            else
                result = Script::Debug::DeleteHardwareBreakpoint(address);
            if(!_plugin_testassert(observer.restore(), "could not restore full-view refresh import"))
                return false;
        }
        if(!_plugin_testassert(result == !missing, "script helper %s returned unexpected success state", argv[1]))
            return false;
        if(!_plugin_testassert(gFullRefreshCalls == (missing ? 0 : 1), "script helper %s full-view refreshes=%ld, expected %d",
                               argv[1], gFullRefreshCalls, missing ? 0 : 1))
            return false;
        if(!functions->BpRefVa(&ref, type, address))
            return false;
        if(remove || removeHw || missing)
            return _plugin_testassert(!functions->BpRefExists(&ref), "script delete left a breakpoint entry");
        return apiState(ref, !disable, !disable) &&
               _plugin_testassert(functions->BpRemove(&ref), "could not clean up script-refresh breakpoint");
    }
}

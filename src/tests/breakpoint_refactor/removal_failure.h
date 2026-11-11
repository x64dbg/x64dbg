#pragma once

#include <delayimp.h>

namespace
{
    LONG gRemovalCalls = 0;

    bool failSoftwareRemoval(ULONG_PTR)
    {
        InterlockedIncrement(&gRemovalCalls);
        return false;
    }

    bool failHardwareRemoval(DWORD)
    {
        InterlockedIncrement(&gRemovalCalls);
        return false;
    }

    bool failMemoryRemoval(ULONG_PTR, SIZE_T)
    {
        InterlockedIncrement(&gRemovalCalls);
        return false;
    }

    // Patch only the debugger's named delay-import slot, never engine code.
    // The debuggee is paused and the patch is scoped to one synchronous command.
    class ScopedRemovalFailure
    {
        void** mSlot = nullptr;
        void* mOriginal = nullptr;
        DWORD mProtection = 0;

    public:
        ScopedRemovalFailure() = default;
        ScopedRemovalFailure(const ScopedRemovalFailure &) = delete;
        ScopedRemovalFailure & operator=(const ScopedRemovalFailure &) = delete;

        ~ScopedRemovalFailure()
        {
            _plugin_testassert(restore(), "failed to restore the engine removal import");
        }

        bool install(BPXTYPE type)
        {
#ifdef _WIN64
            auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x64dbg.dll"));
#else
            auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x32dbg.dll"));
#endif
            return installImport(base, type == bp_normal ? "DeleteBPX" :
                                 type == bp_hardware ? "DeleteHardwareBreakPoint" : "RemoveMemoryBPX",
                                 type == bp_normal ? reinterpret_cast<void*>(&failSoftwareRemoval) :
                                 type == bp_hardware ? reinterpret_cast<void*>(&failHardwareRemoval) :
                                 reinterpret_cast<void*>(&failMemoryRemoval));
        }

        bool installImport(unsigned char* base, const char* name, void* replacement)
        {
            if(!base)
                return false;
            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
            if(!directory.VirtualAddress)
                return false;
            auto descriptor = reinterpret_cast<ImgDelayDescr*>(base + directory.VirtualAddress);
            for(; descriptor->rvaDLLName; descriptor++)
            {
                if(!(descriptor->grAttrs & dlattrRva) ||
                        _stricmp(reinterpret_cast<const char*>(base + descriptor->rvaDLLName), "TitanEngine.dll") != 0)
                    continue;
                auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->rvaINT);
                auto slots = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->rvaIAT);
                for(size_t i = 0; names[i].u1.AddressOfData; i++)
                {
                    if(IMAGE_SNAP_BY_ORDINAL(names[i].u1.Ordinal))
                        continue;
                    auto imported = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names[i].u1.AddressOfData);
                    if(strcmp(reinterpret_cast<const char*>(imported->Name), name) != 0)
                        continue;
                    auto slot = reinterpret_cast<void**>(&slots[i].u1.Function);
                    DWORD protection;
                    if(!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &protection))
                        return false;
                    mProtection = protection;
                    mOriginal = InterlockedExchangePointer(slot, replacement);
                    mSlot = slot;
                    gRemovalCalls = 0;
                    DWORD unused;
                    return VirtualProtect(slot, sizeof(*slot), protection, &unused) != FALSE;
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
            InterlockedExchangePointer(mSlot, mOriginal);
            DWORD unused;
            const bool restored = VirtualProtect(mSlot, sizeof(*mSlot), mProtection, &unused) != FALSE;
            if(restored)
                mSlot = nullptr;
            return restored;
        }
    };

    bool engineBreakpointInstalled(const BRIDGEBP & bp, bool expected)
    {
        bool installed = false;
        if(bp.type == bp_normal)
        {
            unsigned char byte = 0;
            SIZE_T read = 0;
            if(!_plugin_testassert(ReadProcessMemory(DbgGetProcessHandle(), reinterpret_cast<void*>(bp.addr),
                                   &byte, sizeof(byte), &read) && read == sizeof(byte), "could not read raw software breakpoint byte"))
                return false;
            installed = byte == 0xCC;
        }
        else if(bp.type == bp_hardware)
        {
            CONTEXT context{};
            context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            if(!_plugin_testassert(bp.slot < 4 && GetThreadContext(DbgGetThreadHandle(), &context),
                                   "could not read hardware breakpoint slot"))
                return false;
            const duint registers[] = { context.Dr0, context.Dr1, context.Dr2, context.Dr3 };
            installed = (context.Dr7 & (3ULL << (bp.slot * 2))) != 0 && registers[bp.slot] == bp.addr;
        }
        else
        {
            MEMORY_BASIC_INFORMATION info{};
            if(!_plugin_testassert(VirtualQueryEx(DbgGetProcessHandle(), reinterpret_cast<void*>(bp.addr),
                                                  &info, sizeof(info)) == sizeof(info), "could not query memory breakpoint protection"))
                return false;
            installed = (info.Protect & PAGE_GUARD) != 0;
        }
        return _plugin_testassert(installed == expected, "engine breakpoint installed=%d, expected %d", installed, expected);
    }

    bool cbRemovalFailure(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 5, "usage: bprremoval type, address, command, is_delete"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type) || !_plugin_testassert(type == bp_normal || type == bp_hardware || type == bp_memory,
                "unsupported removal failure type"))
            return false;
        const auto address = DbgValFromString(argv[2]);
        const bool isDelete = DbgValFromString(argv[4]) != 0;
        BRIDGEBP before{};
        if(!_plugin_testassert(findBreakpoint(type, address, before) && before.enabled && before.active,
                               "removal test requires an enabled, active breakpoint") || !engineBreakpointInstalled(before, true))
            return false;

        bool result;
        {
            ScopedRemovalFailure failure;
            if(!_plugin_testassert(failure.install(type), "could not install scoped removal failure"))
                return false;
            result = DbgCmdExecDirect(argv[3]);
            if(!_plugin_testassert(failure.restore(), "could not restore removal import"))
                return false;
        }
        if(!_plugin_testassert(gRemovalCalls == 1, "expected one intercepted engine removal, got %ld", gRemovalCalls) ||
                !_plugin_testassert(!result, "failed engine removal must make command '%s' fail", argv[3]) ||
                !engineBreakpointInstalled(before, true))
            return false;
        BRIDGEBP after{};
        if(!_plugin_testassert(findBreakpoint(type, address, after), "failed removal lost the breakpoint database entry") ||
                !_plugin_testassert(after.enabled && after.active, "failed removal cleared enabled/active state") ||
                !_plugin_testassert(after.slot == before.slot, "failed removal changed the hardware slot"))
            return false;

        if(!_plugin_testassert(DbgCmdExecDirect(argv[3]), "retry of '%s' failed after restoring the engine", argv[3]) ||
                !engineBreakpointInstalled(before, false))
            return false;
        const bool exists = findBreakpoint(type, address, after);
        if(isDelete)
            return _plugin_testassert(!exists, "successful delete left a breakpoint database entry");
        return _plugin_testassert(exists && !after.enabled && !after.active, "successful disable did not clear enabled/active state");
    }
}

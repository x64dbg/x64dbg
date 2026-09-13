#include "runtoparty.h"
#include "breakpoint.h"
#include "console.h"
#include "module.h"
#include "thread.h"
#include "threading.h"
#include <algorithm>

namespace
{
    struct PartyRunState
    {
        int party = 0;
        TITANCBSTEP callback = nullptr;
        STEPFUNCTION fallback = StepIntoWow64;
        bool running = false;
        std::vector<std::pair<duint, duint>> breakpoints;
    };

    PartyRunState partyRun;

    // LockRunToUserCode must be held. Only successfully installed breakpoints are
    // owned here; never remove or replace an existing user memory breakpoint.
    void clearPartyRunBreakpoints()
    {
        for(const auto & range : partyRun.breakpoints)
            RemoveMemoryBPX(range.first, range.second);
        partyRun.breakpoints.clear();
        partyRun.running = false;
    }

    void cbPartyRunStep()
    {
        TITANCBSTEP callback;
        STEPFUNCTION fallback;
        {
            EXCLUSIVE_ACQUIRE(LockRunToUserCode);
            if(!partyRun.callback)
                return;
            hActiveThread = ThreadGetHandle(GetDebugData()->dwThreadId);
            auto cip = GetContextDataEx(hActiveThread, UE_CIP);
            callback = partyRun.callback;
            fallback = partyRun.fallback;
            if(ModGetParty(cip) == partyRun.party)
            {
                clearPartyRunBreakpoints();
                partyRun.callback = nullptr;
            }
            else
            {
                // The page's classification changed, or this is the single-step
                // fallback after a module load/unload. Do not report a false hit.
                clearPartyRunBreakpoints();
                callback = nullptr;
            }
        }
        // Release the lock before invoking a callback, which may start another run.
        if(callback)
            callback();
        else
            fallback(cbPartyRunStep);
    }

    void cbPartyRunMemory(const void*)
    {
        cbPartyRunStep();
    }

    bool collectPartyRunRanges(int party, std::vector<std::pair<duint, duint>> & ranges)
    {
        // Memory breakpoints can disguise executable pages by changing protection.
        // Conservatively fall back rather than miss one or disturb a user BP.
        std::vector<BREAKPOINT> breakpoints;
        BpGetList(&breakpoints);
        for(const auto & bp : breakpoints)
            if(bp.type == BPMEMORY && bp.enabled)
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "Run-to-party: an enabled user memory breakpoint prevents setup."));
                return false;
            }

        // VirtualQuery regions can span module boundaries. Split them so that the
        // same ModGetParty classification as single-stepping is used, including
        // executable private/JIT memory (which is user code).
        std::vector<duint> boundaries;
        ModEnum([&](const MODINFO & mod)
        {
            boundaries.push_back(mod.base);
            boundaries.push_back(mod.base + mod.size);
        });
        std::sort(boundaries.begin(), boundaries.end());

        duint address = 0;
        for(;;)
        {
            MEMORY_BASIC_INFORMATION mbi;
            if(!VirtualQueryEx(fdProcessInfo->hProcess, (LPCVOID)address, &mbi, sizeof(mbi)))
            {
                // VirtualQueryEx returns ERROR_INVALID_PARAMETER past the target's
                // address space (also handles large-address-aware WOW64 targets).
                auto error = GetLastError();
                if(error == ERROR_INVALID_PARAMETER)
                    break;
                dprintf(QT_TRANSLATE_NOOP("DBG", "Run-to-party: VirtualQueryEx failed at %p (error %u).\n"), address, error);
                return false;
            }
            auto end = duint(mbi.BaseAddress) + mbi.RegionSize;
            if(end <= address)
            {
                // The final free region can end at 4 GiB and wrap on x86.
                if(mbi.State != MEM_COMMIT)
                    break;
                return false;
            }
            const DWORD executable = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
            if(mbi.State == MEM_COMMIT && (mbi.Protect & executable))
            {
                while(address < end)
                {
                    auto next = std::upper_bound(boundaries.begin(), boundaries.end(), address);
                    auto rangeEnd = next == boundaries.end() ? end : std::min(end, *next);
                    if(ModGetParty(address) == party)
                    {
                        // Do not consume a guard page belonging to the debuggee.
                        if(mbi.Protect & PAGE_GUARD)
                        {
                            dprintf(QT_TRANSLATE_NOOP("DBG", "Run-to-party: target range %p has PAGE_GUARD.\n"), address);
                            return false;
                        }
                        ranges.emplace_back(address, rangeEnd - address);
                    }
                    address = rangeEnd;
                }
            }
            address = end;
        }
        return !ranges.empty();
    }
}

bool RunToParty(int party, TITANCBSTEP callback, STEPFUNCTION fallback)
{
    EXCLUSIVE_ACQUIRE(LockRunToUserCode);
    if(!callback || !fallback || partyRun.callback)
        return false;

    std::vector<std::pair<duint, duint>> ranges;
    if(!collectPartyRunRanges(party, ranges))
        return false;

    for(const auto & range : ranges)
    {
        if(!SetMemoryBPXEx(range.first, range.second, UE_MEMORY_EXECUTE, true, cbPartyRunMemory))
        {
            auto error = GetLastError();
            // Some executable image mappings reject removal of execute access
            // (ERROR_INVALID_ADDRESS). Try the engine's guard/access mechanism
            // instead. Its callback still checks the executing instruction's
            // party; a data access from excluded code falls back to stepping.
            if(error == ERROR_INVALID_ADDRESS && SetMemoryBPXEx(range.first, range.second, UE_MEMORY, true, cbPartyRunMemory))
            {
                partyRun.breakpoints.push_back(range);
                continue;
            }
            MEMORY_BASIC_INFORMATION mbi = {};
            VirtualQueryEx(fdProcessInfo->hProcess, (LPCVOID)range.first, &mbi, sizeof(mbi));
            unsigned char byte = 0;
            auto readable = ReadProcessMemory(fdProcessInfo->hProcess, (LPCVOID)range.first, &byte, sizeof(byte), nullptr);
            dprintf(QT_TRANSLATE_NOOP("DBG", "Run-to-party: execute breakpoint setup failed at %p, size %p (last error %u, allocation %p, protect %X, type %X, readable %u).\n"), range.first, range.second, error, duint(mbi.AllocationBase), mbi.Protect, mbi.Type, unsigned(readable));
            clearPartyRunBreakpoints();
            return false;
        }
        partyRun.breakpoints.push_back(range);
    }
    partyRun.party = party;
    partyRun.callback = callback;
    partyRun.fallback = fallback;
    partyRun.running = true;
    return true;
}

bool RunToPartyIsActive()
{
    SHARED_ACQUIRE(LockRunToUserCode);
    return partyRun.callback != nullptr;
}

void RunToPartyClear()
{
    EXCLUSIVE_ACQUIRE(LockRunToUserCode);
    clearPartyRunBreakpoints();
    partyRun.callback = nullptr;
}

void RunToPartyOnModuleChange()
{
    STEPFUNCTION fallback;
    {
        EXCLUSIVE_ACQUIRE(LockRunToUserCode);
        if(!partyRun.running)
            return;
        fallback = partyRun.fallback;
        clearPartyRunBreakpoints();
    }
    // A loader event is not an instruction-completion event. In particular,
    // WOW64 may report a transition context here. Wait for a real step before
    // checking the party or invoking the trace callback.
    fallback(cbPartyRunStep);
}

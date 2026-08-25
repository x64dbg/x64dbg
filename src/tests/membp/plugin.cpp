#include <Windows.h>

#include <array>
#include <atomic>
#include <cstring>
#include <string>

#include "_plugins.h"
#include "bridgemain.h"


namespace
{
    constexpr size_t HitHistoryCapacity = 32;

    enum class OnHitAction
    {
        None,
        Delete,
        Add,
    };

    int gPluginHandle = 0;
    std::atomic<unsigned int> gMemoryHitCount{ 0 };
    std::atomic<duint> gLastBpAddr{ 0 };
    std::atomic<duint> gLastCip{ 0 };
    std::atomic<bool> gExitObserved{ false };
    std::atomic<unsigned long> gExitCode{ 0 };
    std::atomic<duint> gCachedBpAddr{ 0 };
    std::atomic<bool> gExpectExitHit{ false };
    std::atomic<duint> gExpectedExitBpAddr{ 0 };
    std::array<duint, HitHistoryCapacity> gHitBpHistory{};
    std::array<duint, HitHistoryCapacity> gHitCipHistory{};
    std::atomic<bool> gHitHistoryOverflow{ false };
    std::atomic<OnHitAction> gOnHitAction{ OnHitAction::None };
    std::atomic<duint> gOnHitActionAddr{ 0 };
    std::atomic<duint> gOnHitActionSize{ 0 };
    std::atomic<unsigned int> gOnHitActionType{ static_cast<unsigned int>(mem_access) };
    std::atomic<bool> gOnHitActionArmed{ false };

    void resetState()
    {
        gMemoryHitCount = 0;
        gLastBpAddr = 0;
        gLastCip = 0;
        gExitObserved = false;
        gExitCode = 0;
        gCachedBpAddr = 0;
        gExpectExitHit = false;
        gExpectedExitBpAddr = 0;
        gHitHistoryOverflow = false;
        gOnHitAction = OnHitAction::None;
        gOnHitActionAddr = 0;
        gOnHitActionSize = 0;
        gOnHitActionType = static_cast<unsigned int>(mem_access);
        gOnHitActionArmed = false;
        for(size_t i = 0; i < HitHistoryCapacity; i++)
        {
            gHitBpHistory[i] = 0;
            gHitCipHistory[i] = 0;
        }
    }

    duint evalExpr(const char* expr)
    {
        return expr ? DbgValFromString(expr) : 0;
    }

    duint evalWrapped(const char* prefix, const char* expr, const char* suffix)
    {
        std::string wrapped = std::string(prefix) + expr + suffix;
        return DbgValFromString(wrapped.c_str());
    }

    bool parseBoolArg(const char* value, bool & parsed)
    {
        if(value == nullptr)
            return false;
        if(strcmp(value, "1") == 0 || _stricmp(value, "true") == 0)
        {
            parsed = true;
            return true;
        }
        if(strcmp(value, "0") == 0 || _stricmp(value, "false") == 0)
        {
            parsed = false;
            return true;
        }
        return false;
    }

    bool parseMemTypeArg(const char* value, BPMEMTYPE & type)
    {
        if(value == nullptr)
            return false;
        if(_stricmp(value, "a") == 0 || _stricmp(value, "access") == 0)
        {
            type = mem_access;
            return true;
        }
        if(_stricmp(value, "r") == 0 || _stricmp(value, "read") == 0)
        {
            type = mem_read;
            return true;
        }
        if(_stricmp(value, "w") == 0 || _stricmp(value, "write") == 0)
        {
            type = mem_write;
            return true;
        }
        if(_stricmp(value, "x") == 0 || _stricmp(value, "execute") == 0)
        {
            type = mem_execute;
            return true;
        }
        return false;
    }

    char memTypeChar(BPMEMTYPE type)
    {
        switch(type)
        {
        case mem_access:
            return 'a';
        case mem_read:
            return 'r';
        case mem_write:
            return 'w';
        case mem_execute:
            return 'x';
        default:
            return '?';
        }
    }

    bool findBpByStart(duint expectedStart, BRIDGEBP & foundBp, duint & size)
    {
        BPMAP list{};
        if(DbgGetBpList(bp_memory, &list) == 0)
            return false;

        bool found = false;
        for(int i = 0; i < list.count; i++)
        {
            const auto & bp = list.bp[i];
            if(bp.type != bp_memory)
                continue;
            if(bp.addr != expectedStart)
                continue;

            foundBp = bp;
            size = DbgFunctions()->MemBpSize(bp.addr);
            found = true;
            break;
        }

        BridgeFree(list.bp);
        return found;
    }

    bool findBpByStart(duint expectedStart, duint & size)
    {
        BRIDGEBP bp{};
        return findBpByStart(expectedStart, bp, size);
    }

    unsigned int countMemoryBreakpoints()
    {
        BPMAP list{};
        if(DbgGetBpList(bp_memory, &list) == 0)
            return 0;

        unsigned int count = 0;
        for(int i = 0; i < list.count; i++)
        {
            if(list.bp[i].type == bp_memory)
                count++;
        }

        BridgeFree(list.bp);
        return count;
    }

    bool assertExpectedHit(duint expectedBp, bool requireProcessAlive, duint count);

    void applyOnHitAction(duint currentBpAddr)
    {
        if(!gOnHitActionArmed.exchange(false))
            return;

        const auto action = gOnHitAction.load();
        const auto actionAddr = gOnHitActionAddr.load();
        char command[256] = "";
        switch(action)
        {
        case OnHitAction::Delete:
        {
            const auto deleteAddr = actionAddr != 0 ? actionAddr : currentBpAddr;
            sprintf_s(command, "membpc 0x%llX", static_cast<unsigned long long>(deleteAddr));
            _plugin_testassert(DbgCmdExecAsyncDirect(command), "callback delete failed: %s", command);
            break;
        }
        case OnHitAction::Add:
        {
            const auto addSize = gOnHitActionSize.load();
            const auto addType = static_cast<BPMEMTYPE>(gOnHitActionType.load());
            sprintf_s(command, "bpmrange 0x%llX, 0x%llX, %c",
                      static_cast<unsigned long long>(actionAddr),
                      static_cast<unsigned long long>(addSize),
                      memTypeChar(addType));
            _plugin_testassert(DbgCmdExecAsyncDirect(command), "callback add failed: %s", command);
            break;
        }
        default:
            break;
        }

        gOnHitAction = OnHitAction::None;
        gOnHitActionAddr = 0;
        gOnHitActionSize = 0;
        gOnHitActionType = static_cast<unsigned int>(mem_access);
    }

    void cbPlugin(CBTYPE cbType, void* callbackInfo)
    {
        if(cbType == CB_INITDEBUG)
        {
            resetState();
            return;
        }

        if(cbType == CB_BREAKPOINT)
        {
            const auto info = static_cast<PLUG_CB_BREAKPOINT*>(callbackInfo);
            if(info && info->breakpoint && info->breakpoint->type == bp_memory)
            {
                const auto oldCount = gMemoryHitCount.fetch_add(1);
                const auto bpAddr = info->breakpoint->addr;
                const auto cip = DbgValFromString("cip");
                gLastBpAddr = bpAddr;
                gLastCip = cip;
                if(oldCount < HitHistoryCapacity)
                {
                    gHitBpHistory[oldCount] = bpAddr;
                    gHitCipHistory[oldCount] = cip;
                }
                else
                    gHitHistoryOverflow = true;
                applyOnHitAction(bpAddr);
            }
            return;
        }

        if(cbType == CB_EXITPROCESS)
        {
            const auto info = static_cast<PLUG_CB_EXITPROCESS*>(callbackInfo);
            if(info && info->ExitProcess)
            {
                gExitObserved = true;
                gExitCode = info->ExitProcess->dwExitCode;
                if(gExpectExitHit.load())
                    assertExpectedHit(gExpectedExitBpAddr.load(), false, 1);
            }
        }
    }

    bool cbReset(int, char**)
    {
        resetState();
        return _plugin_testassert(true, "state reset");
    }

    bool cbAssertRegion(int argc, char** argv)
    {
        if(argc < 2)
            return false;

        const duint target = evalExpr(argv[1]);
        if(!_plugin_testassert(target != 0, "failed to resolve target expression '%s'", argv[1]))
            return false;

        const duint expectedStart = evalWrapped("mem.base(", argv[1], ")");
        const duint expectedSize = evalWrapped("mem.size(", argv[1], ")");
        if(!_plugin_testassert(expectedStart != 0, "failed to resolve mem.base(%s)", argv[1]))
            return false;
        if(!_plugin_testassert(expectedSize != 0, "failed to resolve mem.size(%s)", argv[1]))
            return false;

        duint actualSize = 0;
        if(!_plugin_testassert(findBpByStart(expectedStart, actualSize), "failed to find memory breakpoint starting at 0x%llX", static_cast<unsigned long long>(expectedStart)))
            return false;

        return _plugin_testassert(
                   actualSize == expectedSize,
                   "expected region breakpoint [0x%llX, 0x%llX), got [0x%llX, 0x%llX) for target %s",
                   static_cast<unsigned long long>(expectedStart),
                   static_cast<unsigned long long>(expectedStart + expectedSize),
                   static_cast<unsigned long long>(expectedStart),
                   static_cast<unsigned long long>(expectedStart + actualSize),
                   argv[1]
               );
    }

    bool cbAssertExact(int argc, char** argv)
    {
        if(argc < 3)
            return false;

        const duint expectedStart = evalExpr(argv[1]);
        const duint expectedSize = evalExpr(argv[2]);
        if(!_plugin_testassert(expectedStart != 0, "failed to resolve target expression '%s'", argv[1]))
            return false;
        if(!_plugin_testassert(expectedSize != 0, "failed to resolve size expression '%s'", argv[2]))
            return false;

        duint actualSize = 0;
        if(!_plugin_testassert(findBpByStart(expectedStart, actualSize), "failed to find memory breakpoint starting at 0x%llX", static_cast<unsigned long long>(expectedStart)))
            return false;

        return _plugin_testassert(
                   actualSize == expectedSize,
                   "expected exact breakpoint [0x%llX, 0x%llX), got [0x%llX, 0x%llX)",
                   static_cast<unsigned long long>(expectedStart),
                   static_cast<unsigned long long>(expectedStart + expectedSize),
                   static_cast<unsigned long long>(expectedStart),
                   static_cast<unsigned long long>(expectedStart + actualSize)
               );
    }

    bool cbAssertInfo(int argc, char** argv)
    {
        if(argc < 6)
            return false;

        const duint expectedStart = evalExpr(argv[1]);
        const duint expectedSize = evalExpr(argv[2]);
        BPMEMTYPE expectedType = mem_access;
        bool expectedEnabled = false;
        bool expectedSingleshoot = false;
        if(!_plugin_testassert(expectedStart != 0, "failed to resolve breakpoint expression '%s'", argv[1]))
            return false;
        if(!_plugin_testassert(expectedSize != 0, "failed to resolve size expression '%s'", argv[2]))
            return false;
        if(!_plugin_testassert(parseMemTypeArg(argv[3], expectedType), "failed to parse memory breakpoint type '%s'", argv[3]))
            return false;
        if(!_plugin_testassert(parseBoolArg(argv[4], expectedEnabled), "failed to parse enabled flag '%s'", argv[4]))
            return false;
        if(!_plugin_testassert(parseBoolArg(argv[5], expectedSingleshoot), "failed to parse singleshoot flag '%s'", argv[5]))
            return false;

        BRIDGEBP bp{};
        duint actualSize = 0;
        if(!_plugin_testassert(findBpByStart(expectedStart, bp, actualSize), "failed to find memory breakpoint starting at 0x%llX", static_cast<unsigned long long>(expectedStart)))
            return false;
        if(!_plugin_testassert(actualSize == expectedSize, "expected memory breakpoint size 0x%llX, got 0x%llX", static_cast<unsigned long long>(expectedSize), static_cast<unsigned long long>(actualSize)))
            return false;
        if(!_plugin_testassert(bp.typeEx == expectedType, "expected memory breakpoint type %u, got %u", static_cast<unsigned int>(expectedType), static_cast<unsigned int>(bp.typeEx)))
            return false;
        if(!_plugin_testassert(bp.enabled == expectedEnabled, "expected enabled=%d, got %d", expectedEnabled ? 1 : 0, bp.enabled ? 1 : 0))
            return false;
        return _plugin_testassert(bp.singleshoot == expectedSingleshoot, "expected singleshoot=%d, got %d", expectedSingleshoot ? 1 : 0, bp.singleshoot ? 1 : 0);
    }

    bool cbAssertBpCount(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        const auto expectedCount = static_cast<unsigned int>(evalExpr(argv[1]));
        return _plugin_testassert(countMemoryBreakpoints() == expectedCount, "expected %u memory breakpoints, got %u", expectedCount, countMemoryBreakpoints());
    }

    bool cbCacheBp(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        const duint expectedBp = evalExpr(argv[1]);
        if(!_plugin_testassert(expectedBp != 0, "failed to resolve breakpoint expression '%s'", argv[1]))
            return false;
        gCachedBpAddr = expectedBp;
        return _plugin_testassert(true, "cached breakpoint address 0x%llX", static_cast<unsigned long long>(expectedBp));
    }

    bool assertExpectedHit(duint expectedBp, bool requireProcessAlive, duint count)
    {
        if(!_plugin_testassert(expectedBp != 0, "failed to resolve expected breakpoint address"))
            return false;
        if(!_plugin_testassert(gMemoryHitCount.load() == count, "expected exactly %lu memory breakpoint callback, got %u", count, gMemoryHitCount.load()))
            return false;
        if(!_plugin_testassert(gLastBpAddr.load() == expectedBp, "expected memory breakpoint address 0x%llX, got 0x%llX", static_cast<unsigned long long>(expectedBp), static_cast<unsigned long long>(gLastBpAddr.load())))
            return false;
        if(!requireProcessAlive)
            return true;
        return _plugin_testassert(!gExitObserved.load(), "process exited before the memory breakpoint assertion, exitCode=%lu, lastCip=0x%llX", gExitCode.load(), static_cast<unsigned long long>(gLastCip.load()));
    }

    bool cbAssertHit(int argc, char** argv)
    {
        duint expectedBp = 0;
        duint count = 1;
        if(argc >= 2)
            expectedBp = evalExpr(argv[1]);
        if(argc >= 3)
            count = evalExpr(argv[2]);
        if(expectedBp == 0)
            expectedBp = gCachedBpAddr.load();
        return assertExpectedHit(expectedBp, true, count);
    }

    bool cbAssertCount(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        const auto expectedCount = static_cast<unsigned int>(evalExpr(argv[1]));
        return _plugin_testassert(gMemoryHitCount.load() == expectedCount, "expected exactly %u memory breakpoint callbacks, got %u", expectedCount, gMemoryHitCount.load());
    }

    bool cbAssertSeq(int argc, char** argv)
    {
        if(argc < 2)
            return false;

        const auto expectedCount = static_cast<unsigned int>(argc - 1);
        if(!_plugin_testassert(!gHitHistoryOverflow.load(), "memory breakpoint hit history overflowed"))
            return false;
        if(!_plugin_testassert(gMemoryHitCount.load() == expectedCount, "expected exactly %u memory breakpoint callbacks, got %u", expectedCount, gMemoryHitCount.load()))
            return false;

        for(int i = 1; i < argc; i++)
        {
            const auto expectedBp = evalExpr(argv[i]);
            if(!_plugin_testassert(expectedBp != 0, "failed to resolve expected breakpoint expression '%s'", argv[i]))
                return false;
            if(!_plugin_testassert(gHitBpHistory[static_cast<size_t>(i - 1)] == expectedBp,
                                   "expected hit %d at 0x%llX, got 0x%llX (cip=0x%llX)",
                                   i,
                                   static_cast<unsigned long long>(expectedBp),
                                   static_cast<unsigned long long>(gHitBpHistory[static_cast<size_t>(i - 1)]),
                                   static_cast<unsigned long long>(gHitCipHistory[static_cast<size_t>(i - 1)])))
                return false;
        }
        return true;
    }

    bool cbAssertAbsent(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        const duint expectedStart = evalExpr(argv[1]);
        duint actualSize = 0;
        if(!_plugin_testassert(expectedStart != 0, "failed to resolve breakpoint expression '%s'", argv[1]))
            return false;
        return _plugin_testassert(!findBpByStart(expectedStart, actualSize), "expected memory breakpoint at 0x%llX to be absent", static_cast<unsigned long long>(expectedStart));
    }

    bool cbAssertNoHit(int argc, char** argv)
    {
        if(argc < 2)
            return false;

        const duint expectedExitCode = evalExpr(argv[1]);
        if(!_plugin_testassert(gMemoryHitCount.load() == 0, "expected no memory breakpoint callbacks, got %u", gMemoryHitCount.load()))
            return false;
        if(!_plugin_testassert(gExitObserved.load(), "expected process exit to be observed"))
            return false;
        return _plugin_testassert(gExitCode.load() == expectedExitCode, "expected exit code 0x%llX, got 0x%lX", static_cast<unsigned long long>(expectedExitCode), gExitCode.load());
    }

    bool cbAssertExit(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        const duint expectedExitCode = evalExpr(argv[1]);
        if(!_plugin_testassert(gExitObserved.load(), "expected process exit to be observed"))
            return false;
        return _plugin_testassert(gExitCode.load() == expectedExitCode, "expected exit code 0x%llX, got 0x%lX", static_cast<unsigned long long>(expectedExitCode), gExitCode.load());
    }

    bool cbExpectExitHit(int argc, char** argv)
    {
        duint expectedBp = 0;
        if(argc >= 2)
            expectedBp = evalExpr(argv[1]);
        if(expectedBp == 0)
            expectedBp = gCachedBpAddr.load();
        if(!_plugin_testassert(expectedBp != 0, "failed to resolve expected exit breakpoint address"))
            return false;
        gExpectedExitBpAddr = expectedBp;
        gExpectExitHit = true;
        return _plugin_testassert(true, "expecting exit-time memory breakpoint assertion for 0x%llX", static_cast<unsigned long long>(expectedBp));
    }

    bool cbSetStartMode(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        const duint mode = evalExpr(argv[1]);
        const duint startModeAddr = evalExpr("membp:StartMode");
        const DWORD startMode = static_cast<DWORD>(mode);
        if(!_plugin_testassert(startModeAddr != 0, "failed to resolve membp:StartMode"))
            return false;
        return _plugin_testassert(DbgMemWrite(startModeAddr, &startMode, sizeof(startMode)), "failed to write StartMode=%lu", static_cast<unsigned long>(startMode));
    }

    bool cbReenable(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        const auto disableCommand = std::string("membpd ") + argv[1];
        const auto enableCommand = std::string("membpe ") + argv[1];
        if(!_plugin_testassert(DbgCmdExecAsyncDirect(disableCommand.c_str()), "membpd failed for %s", argv[1]))
            return false;
        return _plugin_testassert(DbgCmdExecAsyncDirect(enableCommand.c_str()), "membpe failed for %s", argv[1]);
    }

    bool cbDeleteOnHit(int argc, char** argv)
    {
        duint expectedBp = 0;
        if(argc >= 2)
            expectedBp = evalExpr(argv[1]);
        gOnHitAction = OnHitAction::Delete;
        gOnHitActionAddr = expectedBp;
        gOnHitActionSize = 0;
        gOnHitActionType = static_cast<unsigned int>(mem_access);
        gOnHitActionArmed = true;
        return _plugin_testassert(true, expectedBp != 0 ? "will delete breakpoint 0x%llX on first hit" : "will delete current breakpoint on first hit", static_cast<unsigned long long>(expectedBp));
    }

    bool cbAddOnHit(int argc, char** argv)
    {
        if(argc < 4)
            return false;
        const duint addr = evalExpr(argv[1]);
        const duint size = evalExpr(argv[2]);
        BPMEMTYPE type = mem_access;
        if(!_plugin_testassert(addr != 0, "failed to resolve breakpoint expression '%s'", argv[1]))
            return false;
        if(!_plugin_testassert(size != 0, "failed to resolve size expression '%s'", argv[2]))
            return false;
        if(!_plugin_testassert(parseMemTypeArg(argv[3], type), "failed to parse memory breakpoint type '%s'", argv[3]))
            return false;
        gOnHitAction = OnHitAction::Add;
        gOnHitActionAddr = addr;
        gOnHitActionSize = size;
        gOnHitActionType = static_cast<unsigned int>(type);
        gOnHitActionArmed = true;
        return _plugin_testassert(true, "will add breakpoint 0x%llX size=0x%llX type=%c on first hit", static_cast<unsigned long long>(addr), static_cast<unsigned long long>(size), memTypeChar(type));
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercallback(gPluginHandle, CB_INITDEBUG, cbPlugin);
    _plugin_registercallback(gPluginHandle, CB_BREAKPOINT, cbPlugin);
    _plugin_registercallback(gPluginHandle, CB_EXITPROCESS, cbPlugin);
    _plugin_registercommand(gPluginHandle, "mbreset", cbReset, false);
    _plugin_registercommand(gPluginHandle, "mbassertregion", cbAssertRegion, false);
    _plugin_registercommand(gPluginHandle, "mbassertexact", cbAssertExact, false);
    _plugin_registercommand(gPluginHandle, "mbassertinfo", cbAssertInfo, false);
    _plugin_registercommand(gPluginHandle, "mbassertbpcount", cbAssertBpCount, false);
    _plugin_registercommand(gPluginHandle, "mbcachebp", cbCacheBp, false);
    _plugin_registercommand(gPluginHandle, "mbasserthit", cbAssertHit, false);
    _plugin_registercommand(gPluginHandle, "mbassertcount", cbAssertCount, false);
    _plugin_registercommand(gPluginHandle, "mbassertseq", cbAssertSeq, false);
    _plugin_registercommand(gPluginHandle, "mbassertabsent", cbAssertAbsent, false);
    _plugin_registercommand(gPluginHandle, "mbassertnohit", cbAssertNoHit, false);
    _plugin_registercommand(gPluginHandle, "mbassertexit", cbAssertExit, false);
    _plugin_registercommand(gPluginHandle, "mbexpectexithit", cbExpectExitHit, false);
    _plugin_registercommand(gPluginHandle, "mbsetstartmode", cbSetStartMode, false);
    _plugin_registercommand(gPluginHandle, "mbreenable", cbReenable, false);
    _plugin_registercommand(gPluginHandle, "mbdeleteonhit", cbDeleteOnHit, false);
    _plugin_registercommand(gPluginHandle, "mbaddonhit", cbAddOnHit, false);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

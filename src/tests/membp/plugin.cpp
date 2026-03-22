#include <Windows.h>

#include <atomic>
#include <cstring>
#include <string>

#include "_plugins.h"
#include "bridgemain.h"

namespace
{
    int gPluginHandle = 0;
    std::atomic<unsigned int> gMemoryHitCount{ 0 };
    std::atomic<duint> gLastBpAddr{ 0 };
    std::atomic<duint> gLastCip{ 0 };
    std::atomic<bool> gExitObserved{ false };
    std::atomic<unsigned long> gExitCode{ 0 };
    std::atomic<duint> gCachedBpAddr{ 0 };

    void resetState()
    {
        gMemoryHitCount = 0;
        gLastBpAddr = 0;
        gLastCip = 0;
        gExitObserved = false;
        gExitCode = 0;
        gCachedBpAddr = 0;
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

    bool findBpByStart(duint expectedStart, duint & size)
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

            size = DbgFunctions()->MemBpSize(bp.addr);
            found = true;
            break;
        }

        BridgeFree(list.bp);
        return found;
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
                gMemoryHitCount.fetch_add(1);
                gLastBpAddr = info->breakpoint->addr;
                gLastCip = DbgValFromString("cip");
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
            "expected region breakpoint [%0llX, 0x%llX), got [0x%llX, 0x%llX) for target %s",
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
            "expected exact breakpoint [%0llX, 0x%llX), got [0x%llX, 0x%llX)",
            static_cast<unsigned long long>(expectedStart),
            static_cast<unsigned long long>(expectedStart + expectedSize),
            static_cast<unsigned long long>(expectedStart),
            static_cast<unsigned long long>(expectedStart + actualSize)
        );
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

    bool cbAssertHit(int argc, char** argv)
    {
        duint expectedBp = 0;
        if(argc >= 2)
            expectedBp = evalExpr(argv[1]);
        if(expectedBp == 0)
            expectedBp = gCachedBpAddr.load();
        if(!_plugin_testassert(expectedBp != 0, "failed to resolve expected breakpoint address"))
            return false;
        if(!_plugin_testassert(gMemoryHitCount.load() == 1, "expected exactly 1 memory breakpoint callback, got %u", gMemoryHitCount.load()))
            return false;
        if(!_plugin_testassert(gLastBpAddr.load() == expectedBp, "expected memory breakpoint address 0x%llX, got 0x%llX", static_cast<unsigned long long>(expectedBp), static_cast<unsigned long long>(gLastBpAddr.load())))
            return false;
        return _plugin_testassert(!gExitObserved.load(), "process exited before the memory breakpoint assertion, exitCode=%lu, lastCip=0x%llX", gExitCode.load(), static_cast<unsigned long long>(gLastCip.load()));
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

    bool cbReenable(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        const auto disableCommand = std::string("membpd ") + argv[1];
        const auto enableCommand = std::string("membpe ") + argv[1];
        if(!_plugin_testassert(DbgCmdExecDirect(disableCommand.c_str()), "membpd failed for %s", argv[1]))
            return false;
        return _plugin_testassert(DbgCmdExecDirect(enableCommand.c_str()), "membpe failed for %s", argv[1]);
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), "MembpRegression", _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercallback(gPluginHandle, CB_INITDEBUG, cbPlugin);
    _plugin_registercallback(gPluginHandle, CB_BREAKPOINT, cbPlugin);
    _plugin_registercallback(gPluginHandle, CB_EXITPROCESS, cbPlugin);
    _plugin_registercommand(gPluginHandle, "mbreset", cbReset, false);
    _plugin_registercommand(gPluginHandle, "mbassertregion", cbAssertRegion, false);
    _plugin_registercommand(gPluginHandle, "mbassertexact", cbAssertExact, false);
    _plugin_registercommand(gPluginHandle, "mbcachebp", cbCacheBp, false);
    _plugin_registercommand(gPluginHandle, "mbasserthit", cbAssertHit, false);
    _plugin_registercommand(gPluginHandle, "mbassertnohit", cbAssertNoHit, false);
    _plugin_registercommand(gPluginHandle, "mbreenable", cbReenable, false);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

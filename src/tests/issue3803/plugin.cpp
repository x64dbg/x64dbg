#include <Windows.h>

#include <array>
#include <cstring>

#include "_plugins.h"
#include "bridgemain.h"

namespace
{
    int gPluginHandle = 0;

    bool cbIssue3803Assert(int, char**)
    {
        if(!_plugin_testassert(!DbgCmdExecAsyncDirect("_K0=5"), "scalar expression assignment to _K0 should fail"))
            return false;
        if(!_plugin_testassert(!DbgCmdExecAsyncDirect("_ZMM0=5"), "scalar expression assignment to _ZMM0 should fail"))
            return false;
        if(!_plugin_testassert(!DbgCmdExecAsyncDirect("mov _K0, 5"), "mov _K0, 5 should fail"))
            return false;
        if(!_plugin_testassert(!DbgCmdExecAsyncDirect("mov _ZMM0, 5"), "mov _ZMM0, 5 should fail"))
            return false;
        if(!_plugin_testassert(!DbgValSetScalar("_K0", 5), "DbgValSetScalar should reject raw K registers"))
            return false;
        if(!_plugin_testassert(!DbgValSetScalar("_ZMM0", 5), "DbgValSetScalar should reject raw ZMM registers"))
            return false;

        ULONGLONG opmask = 0x0123456789ABCDEFULL;
        if(!_plugin_testassert(!DbgValSetBuffer("_K0", &opmask, sizeof(DWORD)), "DbgValSetBuffer should require the full K register size"))
            return false;
        if(!_plugin_testassert(DbgValSetBuffer("_K0", &opmask, sizeof(opmask)), "DbgValSetBuffer failed for _K0"))
            return false;

        std::array<unsigned char, sizeof(ZMMREGISTER)> zmm = {};
        for(size_t i = 0; i < zmm.size(); i++)
            zmm[i] = static_cast<unsigned char>(i ^ 0x5A);
        if(!_plugin_testassert(!DbgValSetBuffer("_ZMM0", zmm.data(), sizeof(XMMREGISTER)), "DbgValSetBuffer should require the full ZMM register size"))
            return false;
        if(!_plugin_testassert(DbgValSetBuffer("_ZMM0", zmm.data(), zmm.size()), "DbgValSetBuffer failed for _ZMM0"))
            return false;

        if(!_plugin_testassert(DbgCmdExecAsyncDirect("alloc 40"), "alloc 40 failed"))
            return false;
        const auto result = DbgValFromString("$result");
        if(!_plugin_testassert(result != 0, "alloc did not populate $result"))
            return false;
        if(!_plugin_testassert(DbgCmdExecAsyncDirect("vmovdqu [$result], zmm0"), "vmovdqu [$result], zmm0 failed"))
            return false;
        std::array<unsigned char, sizeof(ZMMREGISTER)> written = {};
        if(!_plugin_testassert(DbgMemRead(result, written.data(), written.size()), "DbgMemRead failed for ZMM spill buffer"))
            return false;
        if(!_plugin_testassert(memcmp(written.data(), zmm.data(), sizeof(XMMREGISTER)) == 0, "ZMM0 low lane mismatch"))
            return false;

        return true;
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercommand(gPluginHandle, "issue3803assert", cbIssue3803Assert, false);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

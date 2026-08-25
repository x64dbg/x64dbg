#include <Windows.h>

#include <cstring>

#include "_plugins.h"
#include "_scriptapi_flag.h"
#include "_scriptapi_misc.h"
#include "bridgemain.h"


namespace
{
    int gPluginHandle = 0;

    struct FlagSnapshot
    {
        bool regdumpOk = false;
        REGDUMP regdump{};
        bool zfExprOk = false;
        duint zfExpr = 0;
        bool cfExprOk = false;
        duint cfExpr = 0;
        bool zfApi = false;
        bool cfApi = false;
    };

    FlagSnapshot TakeSnapshot()
    {
        FlagSnapshot snapshot;
        snapshot.regdumpOk = DbgGetRegDumpEx(reinterpret_cast<REGDUMP_AVX512*>(&snapshot.regdump), sizeof(snapshot.regdump));
        snapshot.zfExprOk = Script::Misc::ParseExpression("_ZF", &snapshot.zfExpr);
        snapshot.cfExprOk = Script::Misc::ParseExpression("_CF", &snapshot.cfExpr);
        snapshot.zfApi = Script::Flag::GetZF();
        snapshot.cfApi = Script::Flag::GetCF();
        return snapshot;
    }

    void LogSnapshot(const char* tag, const FlagSnapshot & snapshot)
    {
        _plugin_logprintf(
            "[issue3808] %s regdump_ok=%d eflags=0x%llX reg_zf=%d reg_cf=%d expr_zf_ok=%d expr_zf=%llu expr_cf_ok=%d expr_cf=%llu api_zf=%d api_cf=%d\n",
            tag,
            snapshot.regdumpOk ? 1 : 0,
            snapshot.regdumpOk ? static_cast<unsigned long long>(snapshot.regdump.regcontext.eflags) : 0ULL,
            snapshot.regdumpOk && snapshot.regdump.flags.z ? 1 : 0,
            snapshot.regdumpOk && snapshot.regdump.flags.c ? 1 : 0,
            snapshot.zfExprOk ? 1 : 0,
            static_cast<unsigned long long>(snapshot.zfExpr),
            snapshot.cfExprOk ? 1 : 0,
            static_cast<unsigned long long>(snapshot.cfExpr),
            snapshot.zfApi ? 1 : 0,
            snapshot.cfApi ? 1 : 0
        );
    }

    bool cbFlagRepro3808(int, char**)
    {
        _plugin_logputs("[issue3808] Priming ZF=1 CF=0 through the command path");
        if(!DbgCmdExecAsyncDirect("_ZF=1") || !DbgCmdExecAsyncDirect("_CF=0"))
            return _plugin_testassert(false, "failed to prime flags through the command path");

        const auto primed = TakeSnapshot();
        LogSnapshot("after_command_path", primed);
        if(!_plugin_testassert(
                    primed.regdumpOk
                    && primed.zfExprOk
                    && primed.cfExprOk
                    && primed.regdump.flags.z
                    && !primed.regdump.flags.c
                    && primed.zfExpr == 1
                    && primed.cfExpr == 0
                    && primed.zfApi
                    && !primed.cfApi,
                    "command path did not prime ZF=1 CF=0 correctly"))
            return false;

        const bool setZfOk = Script::Flag::SetZF(false);
        const auto afterSetZf = TakeSnapshot();
        LogSnapshot("after_SetZF_false", afterSetZf);
        if(!_plugin_testassert(setZfOk, "Script::Flag::SetZF(false) returned failure"))
            return false;

        const bool setCfOk = Script::Flag::SetCF(true);
        const auto finalSnapshot = TakeSnapshot();
        LogSnapshot("after_SetCF_true", finalSnapshot);
        if(!_plugin_testassert(setCfOk, "Script::Flag::SetCF(true) returned failure"))
            return false;

        return _plugin_testassert(
                   finalSnapshot.regdumpOk
                   && finalSnapshot.zfExprOk
                   && finalSnapshot.cfExprOk
                   && !finalSnapshot.regdump.flags.z
                   && finalSnapshot.regdump.flags.c
                   && finalSnapshot.zfExpr == 0
                   && finalSnapshot.cfExpr == 1
                   && !finalSnapshot.zfApi
                   && finalSnapshot.cfApi,
                   "Script::Flag::Set* did not update the final flag state to ZF=0 CF=1");
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercommand(gPluginHandle, "flagrepro3808", cbFlagRepro3808, true);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

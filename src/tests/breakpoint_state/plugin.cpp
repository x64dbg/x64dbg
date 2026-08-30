#include <Windows.h>

#include <array>
#include <cstring>
#include <string>

#include "_plugins.h"
#include "bridgemain.h"

namespace
{
    int gPluginHandle = 0;
    constexpr unsigned int TestException = 0xE0424242;
    constexpr const char* TestDll = "breakpoint-state-probe.dll";

    struct BreakpointCase
    {
        const char* name;
        BP_REF ref{};
        BPXTYPE type = bp_none;
    };

    bool command(const std::string & text)
    {
        return _plugin_testassert(DbgCmdExecDirect(text.c_str()), "command failed: %s", text.c_str());
    }

    std::string hexAddress(duint address)
    {
        char text[32] = {};
        sprintf_s(text, "%llX", static_cast<unsigned long long>(address));
        return text;
    }

    bool readBool(BreakpointCase & test, BP_FIELD field, bool expected, const char* fieldName)
    {
        bool value = !expected;
        const bool read = test.ref.GetField(field, value);
        return _plugin_testassert(read && value == expected,
                                  "%s: expected %s=%d, read=%d value=%d",
                                  test.name, fieldName, expected ? 1 : 0, read ? 1 : 0, value ? 1 : 0);
    }

    bool assertRef(BreakpointCase & test)
    {
        BPXTYPE type = bp_none;
        const bool exists = DbgFunctions()->BpRefExists(&test.ref);
        test.ref.GetField(bpf_type, type);
        return _plugin_testassert(exists && type == test.type,
                                  "%s: invalid reference (exists=%d type=%d expected=%d)",
                                  test.name, exists ? 1 : 0, int(type), int(test.type));
    }

    bool setEnabled(BreakpointCase & test, bool enabled)
    {
        const bool changed = test.ref.SetField(bpf_enabled, duint(enabled));
        if(!_plugin_testassert(changed, "%s: BP_REF failed to set enabled=%d", test.name, enabled ? 1 : 0))
            return false;
        return readBool(test, bpf_enabled, enabled, "enabled");
    }

    bool cbBreakpointStateRegression(int, char**)
    {
        const duint software = DbgValFromString("breakpoint_state:SoftwareSite");
        const duint hardware = DbgValFromString("breakpoint_state:HardwareValue");
        const duint memory = DbgValFromString("breakpoint_state:MemoryValue");
        const duint moduleBase = DbgFunctions()->ModBaseFromAddr(software);
        if(!_plugin_testassert(software && hardware && memory && moduleBase,
                               "failed to resolve breakpoint_state test symbols"))
            return false;

        if(!command("bp breakpoint_state:SoftwareSite") ||
                !command("bphws breakpoint_state:HardwareValue, w, 4") ||
                !command("bpmrange breakpoint_state:MemoryValue, 4, w") ||
                !command(std::string("bpdll ") + TestDll + ", l") ||
                !command("SetExceptionBPX E0424242, first"))
            return false;

        std::array<BreakpointCase, 5> tests =
        {{
                { "software", {}, bp_normal },
                { "hardware", {}, bp_hardware },
                { "memory", {}, bp_memory },
                { "DLL", {}, bp_dll },
                { "exception", {}, bp_exception },
            }};

        DbgFunctions()->BpRefVa(&tests[0].ref, tests[0].type, software);
        DbgFunctions()->BpRefVa(&tests[1].ref, tests[1].type, hardware);
        DbgFunctions()->BpRefVa(&tests[2].ref, tests[2].type, memory);
        DbgFunctions()->BpRefDll(&tests[3].ref, TestDll);
        DbgFunctions()->BpRefException(&tests[4].ref, TestException);

        bool success = true;
        for(auto & test : tests)
        {
            success &= assertRef(test);
            success &= readBool(test, bpf_enabled, true, "enabled");
        }

        const struct
        {
            const char* name;
            BPXTYPE type;
            duint va;
        } relativeCases[] =
        {
            { "software", bp_normal, software },
            { "hardware", bp_hardware, hardware },
            { "memory", bp_memory, memory },
        };

        for(const auto & relative : relativeCases)
        {
            BP_REF ref{};
            const auto rva = relative.va - moduleBase;
            const bool created = DbgFunctions()->BpRefRva(&ref, relative.type, "breakpoint_state.exe", rva);
            const bool exists = created && DbgFunctions()->BpRefExists(&ref);
            success &= _plugin_testassert(created && exists,
                                          "%s: BpRefRva failed (created=%d exists=%d)",
                                          relative.name, created ? 1 : 0, exists ? 1 : 0);
        }

        for(auto & test : tests)
            success &= setEnabled(test, false);
        for(auto & test : tests)
            success &= setEnabled(test, true);

        success &= command(std::string("bc ") + hexAddress(software));
        success &= command(std::string("bphwc ") + hexAddress(hardware));
        success &= command(std::string("bpmc ") + hexAddress(memory));
        success &= command(std::string("bcdll ") + TestDll);
        success &= command("DeleteExceptionBPX E0424242");

        for(const auto & test : tests)
            success &= _plugin_testassert(!DbgFunctions()->BpRefExists(&test.ref), "%s: reference still exists after deletion", test.name);

        return success;
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercommand(gPluginHandle, "bpstateregression", cbBreakpointStateRegression, true);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

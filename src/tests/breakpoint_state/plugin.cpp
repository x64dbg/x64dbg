#include <Windows.h>

#include <array>
#include <cstring>

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
        duint va = 0;
    };

    using BreakpointCases = std::array<BreakpointCase, 5>;

    bool buildReferences(BreakpointCases & tests, duint & moduleBase)
    {
        const duint software = DbgValFromString("breakpoint_state:SoftwareSite");
        const duint hardware = DbgValFromString("breakpoint_state:HardwareValue");
        const duint memory = DbgValFromString("breakpoint_state:MemoryValue");
        moduleBase = DbgFunctions()->ModBaseFromAddr(software);
        if(!_plugin_testassert(software && hardware && memory && moduleBase,
                               "failed to resolve breakpoint_state test symbols"))
            return false;

        tests =
        {
            {
                { "software", {}, bp_normal, software },
                { "hardware", {}, bp_hardware, hardware },
                { "memory", {}, bp_memory, memory },
                { "DLL", {}, bp_dll, 0 },
                { "exception", {}, bp_exception, 0 },
            }
        };

        const bool created =
            DbgFunctions()->BpRefVa(&tests[0].ref, tests[0].type, software) &&
            DbgFunctions()->BpRefVa(&tests[1].ref, tests[1].type, hardware) &&
            DbgFunctions()->BpRefVa(&tests[2].ref, tests[2].type, memory);
        DbgFunctions()->BpRefDll(&tests[3].ref, TestDll);
        DbgFunctions()->BpRefException(&tests[4].ref, TestException);
        return _plugin_testassert(created, "failed to construct address BP_REF values");
    }

    bool readEnabled(BreakpointCase & test, bool expected)
    {
        bool value = !expected;
        const bool read = test.ref.GetField(bpf_enabled, value);
        return _plugin_testassert(read && value == expected,
                                  "%s: expected enabled=%d, read=%d value=%d",
                                  test.name, expected ? 1 : 0, read ? 1 : 0, value ? 1 : 0);
    }

    bool assertPresent(BreakpointCase & test, bool expectedEnabled)
    {
        duint type = bp_none;
        const bool exists = DbgFunctions()->BpRefExists(&test.ref);
        const bool read = test.ref.GetField(bpf_type, type);
        const bool valid = _plugin_testassert(exists && read && type == duint(test.type),
                                              "%s: invalid reference (exists=%d read=%d type=%d expected=%d)",
                                              test.name, exists ? 1 : 0, read ? 1 : 0, int(type), int(test.type));
        return readEnabled(test, expectedEnabled) && valid;
    }

    bool parseEnabled(int argc, char** argv, bool & enabled)
    {
        const bool valid = argc == 2 && (strcmp(argv[1], "0") == 0 || strcmp(argv[1], "1") == 0);
        if(!_plugin_testassert(valid, "expected one enabled argument: 0 or 1"))
            return false;
        enabled = argv[1][0] == '1';
        return true;
    }

    bool cbBpRefAssertPresent(int argc, char** argv)
    {
        bool expectedEnabled = false;
        if(!parseEnabled(argc, argv, expectedEnabled))
            return false;

        BreakpointCases tests;
        duint moduleBase = 0;
        if(!buildReferences(tests, moduleBase))
            return false;

        bool success = true;
        for(auto & test : tests)
            success &= assertPresent(test, expectedEnabled);

        for(auto & test : tests)
        {
            if(test.va == 0)
                continue;
            BP_REF relative{};
            const bool created = DbgFunctions()->BpRefRva(&relative, test.type, "breakpoint_state.exe", test.va - moduleBase);
            const bool exists = created && DbgFunctions()->BpRefExists(&relative);
            success &= _plugin_testassert(created && exists,
                                          "%s: BpRefRva failed (created=%d exists=%d)",
                                          test.name, created ? 1 : 0, exists ? 1 : 0);
        }
        return success;
    }

    bool cbBpRefSetEnabled(int argc, char** argv)
    {
        bool enabled = false;
        if(!parseEnabled(argc, argv, enabled))
            return false;

        BreakpointCases tests;
        duint moduleBase = 0;
        if(!buildReferences(tests, moduleBase))
            return false;

        bool success = true;
        for(auto & test : tests)
        {
            const bool changed = test.ref.SetField(bpf_enabled, duint(enabled));
            success &= _plugin_testassert(changed, "%s: BP_REF failed to set enabled=%d", test.name, enabled ? 1 : 0);
            success &= readEnabled(test, enabled);
        }
        return success;
    }

    bool cbBpRefAssertAbsent(int, char**)
    {
        BreakpointCases tests;
        duint moduleBase = 0;
        if(!buildReferences(tests, moduleBase))
            return false;

        bool success = true;
        for(auto & test : tests)
            success &= _plugin_testassert(!DbgFunctions()->BpRefExists(&test.ref),
                                          "%s: reference still exists after deletion", test.name);
        return success;
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercommand(gPluginHandle, "bprefassertpresent", cbBpRefAssertPresent, true);
    _plugin_registercommand(gPluginHandle, "bprefsetenabled", cbBpRefSetEnabled, true);
    _plugin_registercommand(gPluginHandle, "bprefassertabsent", cbBpRefAssertAbsent, true);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}

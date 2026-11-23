#pragma once

namespace
{
    bool apiRef(BPXTYPE type, duint address, const char* module, BP_REF & ref)
    {
        auto functions = DbgFunctions();
        if(type == bp_dll)
            functions->BpRefDll(&ref, module);
        else if(type == bp_exception)
            functions->BpRefException(&ref, static_cast<unsigned int>(address));
        else if(!functions->BpRefVa(&ref, type, address))
            return false;
        return functions->BpRefExists(&ref);
    }

    bool cbApiEnable(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprapienable type, address|module"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type))
            return false;
        const auto address = type == bp_dll ? 0 : DbgValFromString(argv[2]);
        BP_REF ref{};
        BRIDGEBP before{};
        if(!_plugin_testassert(apiRef(type, address, argv[2], ref) && findBreakpoint(type, address, before, argv[2]),
                               "API test requires an existing breakpoint"))
            return false;
        auto functions = DbgFunctions();
        for(int enabled : { 0, 0, 1, 1 })
        {
            if(!_plugin_testassert(functions->BpSetFieldNumber(&ref, bpf_enabled, enabled),
                                   "API could not set enabled=%d", enabled))
                return false;
            duint actualEnabled = 0, active = 0;
            if(!_plugin_testassert(functions->BpGetFieldNumber(&ref, bpf_enabled, &actualEnabled) &&
                                   functions->BpGetFieldNumber(&ref, bpf_active, &active) &&
                                   actualEnabled == enabled && active == enabled, "API enabled/active state mismatch"))
                return false;
            if((type == bp_normal || type == bp_hardware || type == bp_memory) && !engineBreakpointInstalled(before, enabled != 0))
                return false;
        }
        return true;
    }

    BP_CREATEINFO apiCreateInfo(BPXTYPE type, duint address, const char* module)
    {
        BP_CREATEINFO info{};
        info.size = sizeof(info);
        info.type = type;
        info.address = address;
        info.module = type == bp_dll ? module : nullptr;
        info.name = "API breakpoint";
        info.typeEx = type == bp_hardware ? static_cast<unsigned int>(hw_execute) :
                      type == bp_dll ? static_cast<unsigned int>(dll_unload) :
                      type == bp_exception ? static_cast<unsigned int>(ex_firstchance) : 0;
        info.memsize = type == bp_memory ? 0x1000 : 0;
        return info;
    }

    bool apiState(const BP_REF & ref, bool enabled, bool active)
    {
        duint actualEnabled = 0, actualActive = 0;
        return _plugin_testassert(DbgFunctions()->BpGetFieldNumber(&ref, bpf_enabled, &actualEnabled) &&
                                  DbgFunctions()->BpGetFieldNumber(&ref, bpf_active, &actualActive) &&
                                  actualEnabled == enabled && actualActive == active, "unexpected API enabled/active state");
    }

    bool cbApiCreate(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3 || argc == 4, "usage: bprapicreate type, address|module [,keep]"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type))
            return false;
        const auto address = type == bp_dll ? 0 : DbgValFromString(argv[2]);
        auto info = apiCreateInfo(type, address, argv[2]);
        auto functions = DbgFunctions();
        BP_REF ref{}, duplicate{};
        if(!_plugin_testassert(functions->BpCreate(&info, &ref), "API disabled creation failed") || !apiState(ref, false, false) ||
                !_plugin_testassert(!functions->BpCreate(&info, &duplicate), "API duplicate creation must fail") ||
                !_plugin_testassert(functions->BpRefExists(&ref), "duplicate creation lost the original breakpoint") ||
                !_plugin_testassert(!functions->BpSetFieldNumber(&ref, bpf_active, 1), "active must remain read-only"))
            return false;
        auto invalidRef = ref;
        invalidRef.type = static_cast<BPXTYPE>(bp_normal | bp_hardware);
        if(!_plugin_testassert(!functions->BpRemove(&invalidRef) &&
                               !functions->BpSetFieldNumber(&invalidRef, bpf_enabled, 1) && functions->BpRefExists(&ref),
                               "invalid reference must not mutate a real breakpoint"))
            return false;
        struct TextContext
        {
            BP_REF ref;
            bool changed;
        } context{ ref, false };
        auto callback = [](const char* name, void* userdata)
        {
            auto context = static_cast<TextContext*>(userdata);
            context->changed = strcmp(name, "API breakpoint") == 0 &&
                               DbgFunctions()->BpSetFieldNumber(&context->ref, bpf_enabled, 1);
        };
        if(!_plugin_testassert(functions->BpGetFieldText(&ref, bpf_name, callback, &context) && context.changed,
                               "text callback could not reentrantly enable breakpoint"))
            return false;
        if(!cbApiEnable(3, argv))
            return false;
        if(argc == 4)
            return true;
        BRIDGEBP before{};
        if(!_plugin_testassert(findBreakpoint(type, address, before, argv[2]), "created breakpoint not in list") ||
                !_plugin_testassert(functions->BpRemove(&ref), "API removal failed") ||
                !_plugin_testassert(!functions->BpRefExists(&ref) && !functions->BpRemove(&ref), "API deletion did not remove the entry"))
            return false;
        if(type != bp_dll && type != bp_exception && !engineBreakpointInstalled(before, false))
            return false;
        info.enabled = true;
        if(!_plugin_testassert(functions->BpCreate(&info, &ref), "API enabled creation failed") ||
                !apiState(ref, true, true) ||
                !_plugin_testassert(functions->BpRemove(&ref), "API enabled creation cleanup failed"))
            return false;
        auto invalid = info;
        invalid.size = 0;
        return _plugin_testassert(!functions->BpCreate(&invalid, &ref) && ref.type == bp_none &&
                                  !functions->BpCreate(nullptr, &ref) && !functions->BpRemove(nullptr), "API did not reject invalid inputs");
    }

    bool failSoftwareInstallation(ULONG_PTR, DWORD, TITANCBSOFTBP)
    {
        InterlockedIncrement(&gRemovalCalls);
        return false;
    }

    bool apiRejectInstallation(ScopedRemovalFailure & failure, BPXTYPE type)
    {
#ifdef _WIN64
        auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x64dbg.dll"));
#else
        auto base = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"x32dbg.dll"));
#endif
        return failure.installImport(base, type == bp_normal ? "SetBPX" :
                                     type == bp_hardware ? "SetHardwareBreakPoint" : "SetMemoryBPXEx",
                                     type == bp_normal ? reinterpret_cast<void*>(&failSoftwareInstallation) :
                                     type == bp_hardware ? reinterpret_cast<void*>(&failHardwareInstallation) :
                                     reinterpret_cast<void*>(&failMemoryInstallation));
    }

    bool cbApiFailure(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprapifailure sw|hw|mem, address"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type) || !_plugin_testassert(type == bp_normal || type == bp_hardware || type == bp_memory,
                "failure test requires an engine breakpoint"))
            return false;
        const auto address = DbgValFromString(argv[2]);
        auto functions = DbgFunctions();
        auto info = apiCreateInfo(type, address, nullptr);
        info.enabled = true;
        BP_REF ref{};
        {
            ScopedRemovalFailure failure;
            if(!_plugin_testassert(apiRejectInstallation(failure, type), "could not reject API installation") ||
                    !_plugin_testassert(!functions->BpCreate(&info, &ref), "rejected API creation must fail") ||
                    !_plugin_testassert(gRemovalCalls == 1, "API creation must attempt exactly one installation") ||
                    !_plugin_testassert(failure.restore(), "could not restore API installation"))
                return false;
        }
        BRIDGEBP bp{};
        if(!_plugin_testassert(!findBreakpoint(type, address, bp), "failed API creation left a phantom entry"))
            return false;
        info.enabled = false;
        if(!_plugin_testassert(functions->BpCreate(&info, &ref), "could not create disabled API breakpoint"))
            return false;
        {
            ScopedRemovalFailure failure;
            if(!_plugin_testassert(apiRejectInstallation(failure, type), "could not reject API enable") ||
                    !_plugin_testassert(!functions->BpSetFieldNumber(&ref, bpf_enabled, 1), "rejected API enable must fail") ||
                    !_plugin_testassert(gRemovalCalls == 1, "API enable must attempt exactly one installation") ||
                    !apiState(ref, false, false) ||
                    !_plugin_testassert(failure.restore(), "could not restore API enable"))
                return false;
        }
        if(!_plugin_testassert(functions->BpSetFieldNumber(&ref, bpf_enabled, 1), "retry of API enable failed") ||
                !_plugin_testassert(findBreakpoint(type, address, bp), "enabled API breakpoint missing") ||
                !engineBreakpointInstalled(bp, true))
            return false;
        for(bool remove : { false, true })
        {
            ScopedRemovalFailure failure;
            if(!_plugin_testassert(failure.install(type), "could not reject API removal"))
                return false;
            const auto result = remove ? functions->BpRemove(&ref) : functions->BpSetFieldNumber(&ref, bpf_enabled, 0);
            if(!_plugin_testassert(!result && gRemovalCalls == 1, "rejected API disable/delete must fail after one engine call") ||
                    !apiState(ref, true, true) || !engineBreakpointInstalled(bp, true) ||
                    !_plugin_testassert(failure.restore(), "could not restore API removal"))
                return false;
        }
        return _plugin_testassert(functions->BpRemove(&ref) && !functions->BpRefExists(&ref), "API delete retry failed") &&
               engineBreakpointInstalled(bp, false);
    }

    bool cbApiSingleshoot(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprapisingleshoot type, address|module"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type))
            return false;
        const auto address = type == bp_dll ? 0 : DbgValFromString(argv[2]);
        BP_REF ref{};
        if(!_plugin_testassert(apiRef(type, address, argv[2], ref), "single-shot API breakpoint missing"))
            return false;
        auto functions = DbgFunctions();
        return _plugin_testassert(functions->BpSetFieldNumber(&ref, bpf_singleshoot, 1) &&
                                  functions->BpSetFieldNumber(&ref, bpf_singleshoot, 1), "could not set single-shot through API");
    }

    bool cbApiPending(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 4, "usage: bprapipending type, module, rva"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type))
            return false;
        auto info = apiCreateInfo(type, DbgValFromString(argv[3]), argv[2]);
        info.module = argv[2];
        info.enabled = true;
        BP_REF ref{}, byRva{};
        auto functions = DbgFunctions();
        if(!_plugin_testassert(functions->BpCreate(&info, &ref), "could not create pending API breakpoint") ||
                !apiState(ref, true, false) ||
                !_plugin_testassert(functions->BpRefRva(&byRva, type, argv[2], info.address) && functions->BpRefExists(&byRva),
                                    "could not reference pending breakpoint by RVA") ||
                !_plugin_testassert(functions->BpSetFieldNumber(&ref, bpf_enabled, 0), "could not disable pending breakpoint") ||
                !apiState(ref, false, false) ||
                !_plugin_testassert(functions->BpSetFieldNumber(&ref, bpf_singleshoot, 1) &&
                                    functions->BpSetFieldNumber(&ref, bpf_singleshoot, 0), "could not configure pending single-shot") ||
                !_plugin_testassert(functions->BpSetFieldNumber(&ref, bpf_enabled, 1), "could not enable pending breakpoint"))
            return false;
        return apiState(ref, true, false);
    }

    bool cbApiReconfigureFailure(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 3, "usage: bprapireconfigure sw|mem, address"))
            return false;
        BPXTYPE type;
        if(!parseType(argv[1], type) || !_plugin_testassert(type == bp_normal || type == bp_memory, "unsupported reconfiguration type"))
            return false;
        const auto address = DbgValFromString(argv[2]);
        auto info = apiCreateInfo(type, address, nullptr);
        info.enabled = true;
        auto functions = DbgFunctions();
        BP_REF ref{};
        if(!_plugin_testassert(functions->BpCreate(&info, &ref), "API creation for reconfiguration failed"))
            return false;
        BRIDGEBP bp{};
        if(!_plugin_testassert(findBreakpoint(type, address, bp), "reconfiguration breakpoint missing"))
            return false;
        {
            ScopedRemovalFailure failure;
            if(!_plugin_testassert(failure.install(type), "could not reject reconfiguration removal") ||
                    !_plugin_testassert(!functions->BpSetFieldNumber(&ref, bpf_singleshoot, 1) && gRemovalCalls == 1,
                                        "reconfiguration must fail if removal fails") ||
                    !apiState(ref, true, true) || !engineBreakpointInstalled(bp, true) ||
                    !_plugin_testassert(failure.restore(), "could not restore reconfiguration removal"))
                return false;
        }
        duint singleshoot = 1;
        if(!_plugin_testassert(functions->BpGetFieldNumber(&ref, bpf_singleshoot, &singleshoot) && !singleshoot,
                               "failed removal changed single-shot configuration"))
            return false;
        {
            ScopedRemovalFailure failure;
            if(!_plugin_testassert(apiRejectInstallation(failure, type), "could not reject reconfiguration installation") ||
                    !_plugin_testassert(!functions->BpSetFieldNumber(&ref, bpf_singleshoot, 1) && gRemovalCalls == 2,
                                        "reconfiguration must try installation and restoration") ||
                    !apiState(ref, true, false) || !engineBreakpointInstalled(bp, false) ||
                    !_plugin_testassert(failure.restore(), "could not restore reconfiguration installation"))
                return false;
        }
        if(!_plugin_testassert(functions->BpSetFieldNumber(&ref, bpf_enabled, 1), "could not recover inactive enabled breakpoint") ||
                !apiState(ref, true, true) || !engineBreakpointInstalled(bp, true))
            return false;
        return _plugin_testassert(functions->BpRemove(&ref), "could not remove recovered breakpoint");
    }
}

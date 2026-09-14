#include <windows.h>
#include <cstring>
#include <cwchar>

extern "C"
{
    __declspec(dllexport) volatile LONG gCallbacks = 0;
    __declspec(dllexport) volatile LONG gReachedEnd = 0;
    __declspec(dllexport) volatile LONG gScratchWritten = 0;
    __declspec(dllexport) volatile LONG gProtectionFailures = 0;
    __declspec(dllexport) volatile LONG gRelease = 0;
    __declspec(dllexport) volatile LONG gSpinIterations = 0;
    __declspec(dllexport) volatile LONG gHandled = 0;
    __declspec(dllexport) volatile LONG gLoadSucceeded = 0;
    __declspec(dllexport) volatile LONG gLoadCalls = 0;
    __declspec(dllexport) volatile LONG gModuleWasLoaded = 0;
    __declspec(dllexport) volatile DWORD gProcessId = 0;
    __declspec(dllexport) volatile LONG gWaitEntered = 0;
    __declspec(dllexport) volatile LONG gWaitReturned = 0;
    HANDLE gWakeEvent = nullptr;
    __declspec(dllexport) DWORD* gScratch = nullptr;
    __declspec(dllexport) void* gSystemAddress = nullptr;

    __declspec(dllexport) __declspec(noinline) void Ready()
    {
        gReachedEnd = 0;
    }

    __declspec(dllexport) __declspec(noinline) void UserCallback()
    {
        ++gCallbacks;
    }

    __declspec(dllexport) __declspec(noinline) void TraceEnd()
    {
        ++gReachedEnd;
    }

    __declspec(dllexport) __declspec(noinline) void Finished()
    {
        gReachedEnd += 2;
    }

    __declspec(dllexport) __declspec(noinline) void SpinBegin()
    {
        while(!gRelease)
        {
            ++gSpinIterations;
            YieldProcessor();
        }
    }

    __declspec(dllexport) __declspec(noinline) void WaitBegin()
    {
        gWaitEntered = 1;
        if(WaitForSingleObject(gWakeEvent, INFINITE) == WAIT_OBJECT_0)
            gWaitReturned = 1;
    }

    __declspec(dllexport) __declspec(noinline) void LoadBegin()
    {
        ++gLoadCalls;
        gLoadSucceeded = 0;
        if(auto module = LoadLibraryW(L"trace_party_module.dll"))
        {
            gLoadSucceeded = 1;
            if(FreeLibrary(module))
                gLoadSucceeded = 2;
        }
    }

    __declspec(dllexport) __declspec(noinline) void ExceptionBegin()
    {
        __try
        {
            RaiseException(0xE0424242, 0, 0, nullptr);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            gHandled = 1;
        }
    }
}

static BOOL CALLBACK OnceCallback(PINIT_ONCE, PVOID, PVOID*)
{
    UserCallback();
    return TRUE;
}

extern "C" __declspec(dllexport) __declspec(noinline) void TraceBegin()
{
    for(int i = 0; i < 3; ++i)
    {
        INIT_ONCE once = INIT_ONCE_STATIC_INIT;
        InitOnceExecuteOnce(&once, OnceCallback, nullptr, nullptr);
    }
}

static DWORD protection(void* address)
{
    MEMORY_BASIC_INFORMATION mbi{};
    return VirtualQuery(address, &mbi, sizeof(mbi)) ? mbi.Protect : 0;
}

int main(int argc, char* argv[])
{
    gScratch = static_cast<DWORD*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if(!gScratch)
        return 1;
    gSystemAddress = reinterpret_cast<void*>(&InitOnceExecuteOnce);
    void* pages[] = { reinterpret_cast<void*>(&TraceBegin), gSystemAddress, gScratch };
    DWORD original[] = { protection(pages[0]), protection(pages[1]), protection(pages[2]) };

    gProcessId = GetCurrentProcessId();
    if(argc > 1 && std::strcmp(argv[1], "wait") == 0)
    {
        wchar_t name[80];
        swprintf_s(name, L"Local\\x64dbg_trace_party_%u", DWORD(gProcessId));
        gWakeEvent = CreateEventW(nullptr, TRUE, FALSE, name);
        if(!gWakeEvent)
            return 1;
    }

    // Warm up the import/API path before the test starts. Test execution then
    // follows ordinary calls/returns; scripts never redirect CIP into helpers.
    TraceBegin();
    gCallbacks = 0;
    gModuleWasLoaded = GetModuleHandleW(L"trace_party_module.dll") != nullptr;
    Ready();
    if(argc > 1 && std::strcmp(argv[1], "spin") == 0)
        SpinBegin();
    else if(argc > 1 && std::strcmp(argv[1], "wait") == 0)
        WaitBegin();
    else if(argc > 1 && (std::strcmp(argv[1], "load") == 0 || std::strcmp(argv[1], "load-twice") == 0))
    {
        LoadBegin();
        if(std::strcmp(argv[1], "load-twice") == 0)
            LoadBegin();
    }
    else if(argc > 1 && std::strcmp(argv[1], "exception") == 0)
        ExceptionBegin();
    TraceBegin();
    TraceEnd();

    // A user memory breakpoint on this unused page must survive trace fallback.
    *gScratch = 0x1234;
    gScratchWritten = 1;
    for(int i = 0; i < 3; ++i)
        if(protection(pages[i]) != original[i])
            ++gProtectionFailures;
    Finished();
    if(gWakeEvent)
        CloseHandle(gWakeEvent);
    VirtualFree(gScratch, 0, MEM_RELEASE);
    return 0;
}

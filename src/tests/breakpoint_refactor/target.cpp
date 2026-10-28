#include <Windows.h>

extern "C" __declspec(dllexport) __declspec(noinline) int BreakpointTarget()
{
    static volatile int value = 42;
    return value;
}

extern "C"
{
    __declspec(dllexport) volatile unsigned char* MemoryPage = nullptr;
}

extern "C" __declspec(dllexport) __declspec(noinline) void MemoryReady()
{
    SetLastError(4);
}

extern "C" __declspec(dllexport) __declspec(noinline) void MemoryAccessed()
{
    SetLastError(5);
}

extern "C" __declspec(dllexport) __declspec(noinline) void ApiHit()
{
    SetLastError(2);
}

extern "C" __declspec(dllexport) __declspec(noinline) void Finished()
{
    SetLastError(3);
}

extern "C" __declspec(dllexport) __declspec(noinline) void Unloaded()
{
    SetLastError(0);
}

extern "C" __declspec(dllexport) __declspec(noinline) void Reloaded()
{
    SetLastError(1);
}

int main()
{
    if(BreakpointTarget() != 42)
        return 1;
    if(MemoryPage)
    {
        MemoryReady();
        MemoryPage[0x1234] = 42;
        MemoryAccessed();
    }
    ApiHit();
    ApiHit();
    // Review regressions consume a one-shot DLL breakpoint during this load.
    auto module = LoadLibraryW(L"winmm.dll");
    if(!module)
        return 2;
    FreeLibrary(module);
    if(GetModuleHandleW(L"winmm.dll"))
        return 3;
    Unloaded();
    module = LoadLibraryW(L"winmm.dll");
    if(!module)
        return 4;
    Reloaded();
    auto timeGetTime = reinterpret_cast<DWORD(WINAPI*)()>(GetProcAddress(module, "timeGetTime"));
    if(!timeGetTime)
        return 5;
    timeGetTime();
    FreeLibrary(module);
    Finished();
    return 0;
}

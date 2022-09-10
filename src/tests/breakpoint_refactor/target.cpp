#include <Windows.h>

extern "C" __declspec(dllexport) __declspec(noinline) int BreakpointTarget()
{
    return 42;
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
    FreeLibrary(module);
    return 0;
}

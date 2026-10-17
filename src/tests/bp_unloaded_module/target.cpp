#include <windows.h>

typedef DWORD(WINAPI* TimeGetTime)();

static volatile LONG gPhase = 0;

extern "C" __declspec(dllexport) __declspec(noinline) void Loaded()
{
    InterlockedExchange(&gPhase, 1);
}

extern "C" __declspec(dllexport) __declspec(noinline) void Unloaded()
{
    InterlockedExchange(&gPhase, 2);
}

int main()
{
    auto winmm = LoadLibraryW(L"winmm.dll");
    if(!winmm)
        return 101;
    Loaded();

    FreeLibrary(winmm);
    if(GetModuleHandleW(L"winmm.dll"))
        return 102;
    Unloaded();

    winmm = LoadLibraryW(L"winmm.dll");
    if(!winmm)
        return 103;
    auto timeGetTime = (TimeGetTime)GetProcAddress(winmm, "timeGetTime");
    if(!timeGetTime)
        return 104;
    timeGetTime();
    FreeLibrary(winmm);
    return 0;
}

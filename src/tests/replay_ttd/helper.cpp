#include <windows.h>

extern "C" __declspec(dllexport) volatile LONG gReplayTtdHelperCalls = 0;

extern "C" __declspec(dllexport) __declspec(noinline)
LONG WINAPI ReplayTtdHelperTransform(LONG value)
{
    InterlockedIncrement(&gReplayTtdHelperCalls);
    return (value ^ 0x5A5A) + 0x123;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, void*)
{
    return TRUE;
}

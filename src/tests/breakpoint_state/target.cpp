#include <Windows.h>

extern "C"
{
    __declspec(dllexport) volatile LONG HardwareValue = 0;
    __declspec(dllexport) volatile LONG MemoryValue = 0;

    __declspec(dllexport) __declspec(noinline) void SoftwareSite()
    {
        MemoryBarrier();
    }
}

int main()
{
    SoftwareSite();
    InterlockedIncrement(&HardwareValue);
    InterlockedIncrement(&MemoryValue);
    return 0;
}

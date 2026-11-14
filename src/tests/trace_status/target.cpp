#include <windows.h>

extern "C" __declspec(dllexport) volatile LONG gCounter = 0;

int main()
{
    for(int i = 0; i < 1000; i++)
        InterlockedIncrement(&gCounter);
    return 0;
}

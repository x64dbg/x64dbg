#include <windows.h>

char global[10] = "0";

int main()
{
    GetTickCount();
    char* a = 0;

    GetCurrentProcessId();
    GetCurrentProcess();
    DWORD old = 0;
    VirtualProtect(global, 0x1000, PAGE_READWRITE | PAGE_GUARD, &old);

    /*throw exceptions*/
    global[0] = 0; //PAGE_GUARD
    *a = 0; //ACCESS_VIOLATION
    asm("int3"); //BREAKPOINT
    return 0;
}

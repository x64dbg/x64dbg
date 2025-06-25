#include <windows.h>
#include <stdio.h>

// Exported test function
extern "C" void __declspec(dllexport) TestFunction(const char* message)
{
    printf("TestFunction: %s\n", message);
}

// DLL entry point
extern "C" BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Actions when DLL is loaded
        printf("Random DLL loaded! Instance: 0x%p\n", hinstDLL);
    }
    break;

    case DLL_PROCESS_DETACH:
        // Actions when DLL is unloaded
        printf("Random DLL unloaded!\n");
        break;

    case DLL_THREAD_ATTACH:
        // Actions when a new thread is created
        break;

    case DLL_THREAD_DETACH:
        // Actions when a thread is destroyed
        break;
    }

    return TRUE; // Successful
}
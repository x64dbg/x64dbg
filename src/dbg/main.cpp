/**
 @file main.cpp

 @brief Implements the main class.
 */

#include "_global.h"

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
        hInst = hinstDLL;
    return TRUE;
}

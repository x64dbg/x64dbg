/**
 @file main.cpp

 @brief Implements the main class.
 */

#include "_global.h"

/**
 @fn extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)

 @brief DLL main.

 @param hinstDLL    The hinst DLL.
 @param fdwReason   The fdw reason.
 @param lpvReserved The lpv reserved.

 @return An APIENTRY.
 */

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
        hInst = hinstDLL;
    return TRUE;
}

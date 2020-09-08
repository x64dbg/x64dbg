/**
 @file main.cpp

 @brief Implements the main class.
 */

#include "debugger.h"

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
    {
        hInst = hinstDLL;

        // Get program directory
        {
            wchar_t wszDir[deflen] = L"";
            if(GetModuleFileNameW(hInst, wszDir, deflen))
            {
                strcpy_s(szProgramDir, StringUtils::Utf16ToUtf8(wszDir).c_str());

                int len = (int)strlen(szProgramDir);
                while(szProgramDir[len] != '\\')
                    len--;
                szProgramDir[len] = 0;
            }
        }

        // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-disablethreadlibrarycalls
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

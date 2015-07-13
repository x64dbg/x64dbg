/**
 @file x64_dbg_exe.cpp

 @brief Implements the 64 debug executable class.
 */

#include <stdio.h>
#include <windows.h>
#include "crashdump.h"
#include "..\x64_dbg_bridge\bridgemain.h"

/**
 @fn int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)

 @brief Window main.

 @param hInstance     The instance.
 @param hPrevInstance The previous instance.
 @param lpCmdLine     The command line.
 @param nShowCmd      The show command.

 @return An APIENTRY.
 */

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    CrashDumpInitialize();

    const char* errormsg = BridgeInit();
    if(errormsg)
    {
        MessageBoxA(0, errormsg, "BridgeInit Error", MB_ICONERROR | MB_SYSTEMMODAL);
        return 1;
    }
    errormsg = BridgeStart();
    if(errormsg)
    {
        MessageBoxA(0, errormsg, "BridgeStart Error", MB_ICONERROR | MB_SYSTEMMODAL);
        return 1;
    }
    return 0;
}

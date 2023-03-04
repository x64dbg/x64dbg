/**
 @file x64dbg_exe.cpp

 @brief Implements the 64 debug executable class.
 */

#include <stdio.h>
#include <windows.h>
#include "crashdump.h"
#include "../bridge/bridgemain.h"
#include "LoadResourceString.h"
#include "signaturecheck.h"

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
    // Reference: https://www.trendmicro.com/pl_pl/research/23/b/investigating-the-plugx-trojan-disguised-as-a-legitimate-windows.html
    if(!InitializeSignatureCheck())
        MessageBoxA(nullptr, "Failed to initialize signature check.", "Error", MB_SYSTEMMODAL | MB_ICONERROR);

    CrashDumpInitialize();

    const wchar_t* errormsg = BridgeInit();
    if(errormsg)
    {
        MessageBoxW(0, errormsg, LoadResString(IDS_BRIDGEINITERR), MB_ICONERROR | MB_SYSTEMMODAL);
        return 1;
    }
    errormsg = BridgeStart();
    if(errormsg)
    {
        MessageBoxW(0, errormsg, LoadResString(IDS_BRIDGESTARTERR), MB_ICONERROR | MB_SYSTEMMODAL);
        return 1;
    }
    return 0;
}

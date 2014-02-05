#include <stdio.h>
#include <windows.h>
#include "resource.h"
#include "..\x64_dbg_bridge\bridgemain.h"
#include "..\x64_dbg_crash\x64_dbg_crash.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    InitCrashHandler();
    const char* errormsg=BridgeInit();
    if(errormsg)
    {
        MessageBoxA(0, errormsg, "BridgeInit Error", MB_ICONERROR|MB_SYSTEMMODAL);
        return 1;
    }
    errormsg=BridgeStart();
    if(errormsg)
    {
        MessageBoxA(0, errormsg, "BridgeStart Error", MB_ICONERROR|MB_SYSTEMMODAL);
        return 1;
    }
    return 0;
}

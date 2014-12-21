#include <stdio.h>
#include <windows.h>
#include "..\x64_dbg_bridge\bridgemain.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
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

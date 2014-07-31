#include <stdio.h>
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    MessageBoxA(0, "This is a launcher for x64_dbg!", "x64_dbg", MB_ICONINFORMATION|MB_SYSTEMMODAL);
    return 0;
}
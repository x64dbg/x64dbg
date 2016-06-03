#include "MiscUtil.h"
#include <windows.h>

void SetApplicationIcon(WId winId)
{
    HICON hIcon = LoadIcon(GetModuleHandleW(0), MAKEINTRESOURCE(100));
    SendMessageW((HWND)winId, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    DestroyIcon(hIcon);
}

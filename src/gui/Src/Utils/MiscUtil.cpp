#include "MiscUtil.h"
#include <windows.h>

void SetApplicationIcon(WId winId)
{
    HICON hIcon = LoadIcon(GetModuleHandleW(0), MAKEINTRESOURCE(100));
    SendMessageW((HWND)winId, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    DestroyIcon(hIcon);
}

QByteArray& ByteReverse(QByteArray& array)
{
    int length = array.length();
    for(int i = 0; i < length / 2; i++)
    {
        char temp = array[i];
        array[i] = array[length - i - 1];
        array[length - i - 1] = temp;
    }
    return array;
}

#include <Windows.h>

HMODULE WINAPI LoadLibraryCheckedW(const wchar_t* szDll, bool allowFailure)
{
    return LoadLibraryW(szDll);
}

HMODULE WINAPI LoadLibraryCheckedA(const char* szDll, bool allowFailure)
{
    return LoadLibraryA(szDll);
}

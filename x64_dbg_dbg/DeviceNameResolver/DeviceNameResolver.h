#ifndef _DEVICENAMERESOLVER_H
#define _DEVICENAMERESOLVER_H

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

__declspec(dllexport) bool DevicePathToPathW(const wchar_t* szDevicePath, wchar_t* szPath, size_t nSize);
__declspec(dllexport) bool DevicePathToPathA(const char* szDevicePath, char* szPath, size_t nSize);
__declspec(dllexport) bool DevicePathFromFileHandleW(HANDLE hFile, wchar_t* szDevicePath, size_t nSize);
__declspec(dllexport) bool DevicePathFromFileHandleA(HANDLE hFile, char* szDevicePath, size_t nSize);
__declspec(dllexport) bool PathFromFileHandleW(HANDLE hFile, wchar_t* szPath, size_t nSize);
__declspec(dllexport) bool PathFromFileHandleA(HANDLE hFile, char* szPath, size_t nSize);

#ifdef __cplusplus
}
#endif

#endif // _DEVICENAMERESOLVER_H

#ifndef _DEVICENAMERESOLVER_H
#define _DEVICENAMERESOLVER_H

#ifdef __cplusplus
extern "C"
{
#endif

__declspec(dllexport) bool DevicePathToPathW(const wchar_t* szDevicePath, wchar_t* szPath, size_t nSize);
__declspec(dllexport) bool DevicePathToPathA(const char* szDevicePath, char* szPath, size_t nSize);

#ifdef __cplusplus
}
#endif

#endif // _DEVICENAMERESOLVER_H

#pragma once

#ifdef _WIN64
#define _WIN32_WINNT 0x0502 // XP x64 is version 5.2
#else
#define _WIN32_WINNT 0x0501
#endif

#ifdef WINVER // Overwrite WINVER if given on command line
#undef WINVER
#endif
#define WINVER _WIN32_WINNT

#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif //_WIN32_IE

#include "ntdll/ntdll.h"

#include "../dbg_types.h"
#include "../dbg_assert.h"
#include "../bridge/bridgemain.h"
#include "stringutils.h"

#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#endif //DLL_IMPORT
#ifndef DLL_IMPORT
#define DLL_IMPORT __declspec(dllimport)
#endif //DLL_IMPORT
#ifndef QT_TRANSLATE_NOOP
#define QT_TRANSLATE_NOOP(context, source) source
#endif //QT_TRANSLATE_NOOP
// Uncomment the following line to allow memory leak tracing
//#define ENABLE_MEM_TRACE

//defines
#define deflen 1024

//superglobal variables
extern HINSTANCE hInst;

//functions
#ifdef ENABLE_MEM_TRACE
void* emalloc(size_t size, const char* reason = "emalloc:???");
void* erealloc(void* ptr, size_t size, const char* reason = "erealloc:???");
void efree(void* ptr, const char* reason = "efree:???");
#else
void* emalloc(size_t size, const char* reason = nullptr);
void* erealloc(void* ptr, size_t size, const char* reason = nullptr);
void efree(void* ptr, const char* reason = nullptr);
#endif //ENABLE_MEM_TRACE
void* json_malloc(size_t size);
void json_free(void* ptr);
int memleaks();
void setalloctrace(const char* file);
bool scmp(const char* a, const char* b);
bool FileExists(const char* file);
bool DirExists(const char* dir);
bool GetFileNameFromHandle(HANDLE hFile, char* szFileName, size_t nCount);
bool GetFileNameFromProcessHandle(HANDLE hProcess, char* szFileName, size_t nCount);
bool GetFileNameFromModuleHandle(HANDLE hProcess, HMODULE hModule, char* szFileName, size_t nCount);
bool settingboolget(const char* section, const char* name);
bool IsWow64();
bool ResolveShortcut(HWND hwnd, const wchar_t* szShortcutPath, std::wstring & executable, std::wstring & arguments, std::wstring & workingDir);
void WaitForThreadTermination(HANDLE hThread, DWORD timeout = INFINITE);
void WaitForMultipleThreadsTermination(const HANDLE* hThread, int count, DWORD timeout = INFINITE);
duint GetThreadCount();

#ifdef _WIN64
#define ArchValue(x32value, x64value) x64value
#else
#define ArchValue(x32value, x64value) x32value
#endif //_WIN64

inline bool detectAVX512()
{
    int EABCDX[4];
    __cpuid(EABCDX, 7); // detect AVX-512
    if(EABCDX[1] & (1 << 16))  // EBX.bit16=1, supports AVX-512
        return true;
    else
        return false;
}

#include "dynamicmem.h"

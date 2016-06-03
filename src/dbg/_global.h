#ifndef _GLOBAL_H
#define _GLOBAL_H

#define _WIN32_WINNT 0x0501
#define WINVER 0x0501
#define _WIN32_IE 0x0500

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <tlhelp32.h>
#include "..\dbg_types.h"
#include "..\dbg_assert.h"
#include "..\bridge\bridgemain.h"
#include "jansson\jansson.h"
#include "jansson\jansson_x64dbg.h"
#include "yara\yara.h"
#include "DeviceNameResolver\DeviceNameResolver.h"
#include "handle.h"
#include "stringutils.h"
#include "dbghelp_safe.h"

#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#endif //DLL_IMPORT
#ifndef DLL_IMPORT
#define DLL_IMPORT __declspec(dllimport)
#endif //DLL_IMPORT

//defines
#define deflen 1024

#ifdef _WIN64 //defined by default
#define fhex "%.16llX"
#define fext "ll"
#else
#define fhex "%.8X"
#define fext ""
#endif // _WIN64

enum arch
{
    notfound,
    invalid,
    x32,
    x64
};

//superglobal variables
extern HINSTANCE hInst;

//functions
void* emalloc(size_t size, const char* reason = "emalloc:???");
void* erealloc(void* ptr, size_t size, const char* reason = "erealloc:???");
void efree(void* ptr, const char* reason = "efree:???");
void* json_malloc(size_t size);
void json_free(void* ptr);
int memleaks();
void setalloctrace(const char* file);
bool arraycontains(const char* cmd_list, const char* cmd);
bool scmp(const char* a, const char* b);
void formathex(char* string);
void formatdec(char* string);
bool FileExists(const char* file);
bool DirExists(const char* dir);
bool GetFileNameFromHandle(HANDLE hFile, char* szFileName);
bool GetFileNameFromProcessHandle(HANDLE hProcess, char* szFileName);
bool settingboolget(const char* section, const char* name);
arch GetFileArchitecture(const char* szFileName);
bool IsWow64();
bool ResolveShortcut(HWND hwnd, const wchar_t* szShortcutPath, char* szResolvedPath, size_t nSize);
void WaitForThreadTermination(HANDLE hThread);

#include "dynamicmem.h"

#endif // _GLOBAL_H

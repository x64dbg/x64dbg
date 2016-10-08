#ifndef _GLOBAL_H
#define _GLOBAL_H

#define _WIN32_WINNT 0x0501

#ifdef WINVER // Overwrite WINVER if given on command line
#undef WINVER
#endif
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
#include <unordered_set>
#include <tlhelp32.h>
#include "../dbg_types.h"
#include "../dbg_assert.h"
#include "../bridge\bridgemain.h"
#include "jansson/jansson.h"
#include "jansson/jansson_x64dbg.h"
#include "DeviceNameResolver/DeviceNameResolver.h"
#include "handle.h"
#include "stringutils.h"
#include "dbghelp_safe.h"

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

enum arch
{
    notfound,
    invalid,
    x32,
    x64,
    dotnet
};

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

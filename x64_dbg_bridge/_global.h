#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <windows.h>
#include "bridgemain.h"

#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#endif //DLL_IMPORT
#ifndef DLL_EXPORT
#define DLL_IMPORT __declspec(dllimport)
#endif //DLL_IMPORT

#ifdef _WIN64 //defined by default
#define fhex "%.16llX"
#define fext "ll"
#define uint unsigned long long
#define sint long long
#else
#define fhex "%.8X"
#define fext ""
#define uint unsigned long
#define sint long
#endif // _WIN64

//hInst GUI/DBG
extern HINSTANCE hInstGui;
extern HINSTANCE hInstDbg;

//GUI typedefs
typedef int (*GUIGUIINIT)(int, char**);
typedef void (*GUIDISASSEMBLEAT)(duint va, duint cip);
typedef void (*GUISETDEBUGSTATE)(DBGSTATE state);
typedef void (*GUIADDLOGMESSAGE)(const char* msg);
typedef void (*GUILOGCLEAR)();
typedef void (*GUIUPDATEREGISTERVIEW)();

//GUI functions
extern GUIGUIINIT _gui_guiinit;
extern GUIDISASSEMBLEAT _gui_disassembleat;
extern GUISETDEBUGSTATE _gui_setdebugstate;
extern GUIADDLOGMESSAGE _gui_addlogmessage;
extern GUILOGCLEAR _gui_logclear;
extern GUIUPDATEREGISTERVIEW _gui_updateregisterview;

//DBG typedefs
typedef const char* (*DBGDBGINIT)();
typedef duint (*DBGMEMFINDBASEADDR)(duint addr, duint* size);
typedef bool (*DBGMEMREAD)(duint addr, unsigned char* dest, duint size, duint* read);
typedef bool (*DBGDBGCMDEXEC)(const char* cmd);
typedef bool (*DBGMEMMAP)(MEMMAP* memmap);
typedef void (*DBGDBGEXITSIGNAL)();
typedef bool (*DBGVALFROMSTRING)(const char* string, duint* value);
typedef bool (*DBGISDEBUGGING)();
typedef bool (*DBGISJUMPGOINGTOEXECUTE)(duint addr);
typedef bool (*DBGADDRINFOGET)(duint addr, SEGMENTREG segment, ADDRINFO* addrinfo);
typedef bool (*DBGADDRINFOSET)(duint addr, ADDRINFO* addrinfo);
typedef BPXTYPE (*DBGBPGETTYPEAT)(duint addr);
typedef bool (*DBGGETREGDUMP)(REGDUMP* regdump);
typedef bool (*DBGVALTOSTRING)(const char* string, duint* value);

//DBG functions
extern DBGDBGINIT _dbg_dbginit;
extern DBGMEMFINDBASEADDR _dbg_memfindbaseaddr;
extern DBGMEMREAD _dbg_memread;
extern DBGDBGCMDEXEC _dbg_dbgcmdexec;
extern DBGMEMMAP _dbg_memmap;
extern DBGDBGEXITSIGNAL _dbg_dbgexitsignal;
extern DBGVALFROMSTRING _dbg_valfromstring;
extern DBGISDEBUGGING _dbg_isdebugging;
extern DBGISJUMPGOINGTOEXECUTE _dbg_isjumpgoingtoexecute;
extern DBGADDRINFOGET _dbg_addrinfoget;
extern DBGADDRINFOSET _dbg_addrinfoset;
extern DBGBPGETTYPEAT _dbg_bpgettypeat;
extern DBGGETREGDUMP _dbg_getregdump;
extern DBGVALTOSTRING _dbg_valtostring;

#endif // _GLOBAL_H

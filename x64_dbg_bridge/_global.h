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
typedef void* (*GUISENDMESSAGE)(MSGTYPE type, void* param1, void* param2);

//GUI functions
extern GUIGUIINIT _gui_guiinit;
extern GUISENDMESSAGE _gui_sendmessage;

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
typedef bool (*DBGMEMISVALIDREADPTR)(duint addr);
typedef int (*DBGGETBPLIST)(BPXTYPE type, BPMAP* bplist);
typedef bool (*DBGDBGCMDEXECDIRECT)(const char* cmd);
typedef uint (*DBGGETBRANCHDESTINATION)(uint addr);

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
extern DBGMEMISVALIDREADPTR _dbg_memisvalidreadptr;
extern DBGGETBPLIST _dbg_getbplist;
extern DBGDBGCMDEXECDIRECT _dbg_dbgcmddirectexec;
extern DBGGETBRANCHDESTINATION _dbg_getbranchdestination;

#endif // _GLOBAL_H

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <windows.h>
#include "bridgemain.h"

//GUI typedefs
typedef int (*GUIGUIINIT)(int, char**);
typedef void* (*GUISENDMESSAGE)(GUIMSG type, void* param1, void* param2);
typedef const char* (*GUITRANSLATETEXT)(const char* source);

//GUI functions
extern GUIGUIINIT _gui_guiinit;
extern GUISENDMESSAGE _gui_sendmessage;
extern GUITRANSLATETEXT _gui_translate_text;

//DBG typedefs
typedef const char* (*DBGDBGINIT)(bool blocking);
typedef duint(*DBGMEMFINDBASEADDR)(duint addr, duint* size);
typedef bool (*DBGMEMREAD)(duint addr, void* dest, duint size, duint* read);
typedef bool (*DBGMEMWRITE)(duint addr, const void* src, duint size, duint* written);
typedef bool (*DBGDBGCMDEXECASYNC)(const char* cmd);
typedef bool (*DBGMEMMAP)(MEMMAP* memmap);
typedef void (*DBGDBGEXITSIGNAL)();
typedef bool (*DBGVALFROMSTRING)(const char* string, duint* value);
typedef bool (*DBGISDEBUGGING)();
typedef bool (*DBGISJUMPGOINGTOEXECUTE)(duint addr);
typedef bool (*DBGADDRINFOGET)(duint addr, SEGMENTREG segment, BRIDGE_ADDRINFO* addrinfo);
typedef bool (*DBGADDRINFOSET)(duint addr, BRIDGE_ADDRINFO* addrinfo);
typedef bool(*DBGENCODETYPESET)(duint addr, duint size, ENCODETYPE type);
typedef BPXTYPE(*DBGBPGETTYPEAT)(duint addr);
typedef bool (*DBGGETREGDUMP)(REGDUMP_AVX512* regdump);
typedef bool (*DBGVALSETBUFFER)(const char* string, const void* data, size_t size);
typedef bool (*DBGVALSETSCALAR)(const char* string, duint value);
typedef bool (*DBGMEMISVALIDREADPTR)(duint addr);
typedef int (*DBGGETBPLIST)(BPXTYPE type, BPMAP* bplist);
typedef bool (*DBGDBGCMDEXECASYNCDIRECT)(const char* cmd);
typedef duint(*DBGGETBRANCHDESTINATION)(duint addr);
typedef duint(*DBGSENDMESSAGE)(DBGMSG type, void* param1, void* param2);

//DBG functions
extern DBGDBGINIT _dbg_dbginit;
extern DBGMEMFINDBASEADDR _dbg_memfindbaseaddr;
extern DBGMEMREAD _dbg_memread;
extern DBGMEMWRITE _dbg_memwrite;
extern DBGDBGCMDEXECASYNC _dbg_dbgcmdexecasync;
extern DBGMEMMAP _dbg_memmap;
extern DBGDBGEXITSIGNAL _dbg_dbgexitsignal;
extern DBGVALFROMSTRING _dbg_valfromstring;
extern DBGISDEBUGGING _dbg_isdebugging;
extern DBGISJUMPGOINGTOEXECUTE _dbg_isjumpgoingtoexecute;
extern DBGADDRINFOGET _dbg_addrinfoget;
extern DBGADDRINFOSET _dbg_addrinfoset;
extern DBGENCODETYPESET _dbg_encodetypeset;
extern DBGBPGETTYPEAT _dbg_bpgettypeat;
extern DBGGETREGDUMP _dbg_getregdump;
extern DBGVALSETBUFFER _dbg_valsetbuffer;
extern DBGVALSETSCALAR _dbg_valsetscalar;
extern DBGMEMISVALIDREADPTR _dbg_memisvalidreadptr;
extern DBGGETBPLIST _dbg_getbplist;
extern DBGDBGCMDEXECASYNCDIRECT _dbg_dbgcmddirectexec;
extern DBGGETBRANCHDESTINATION _dbg_getbranchdestination;
extern DBGSENDMESSAGE _dbg_sendmessage;

#endif // _GLOBAL_H

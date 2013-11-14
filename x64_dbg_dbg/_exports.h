#ifndef _EXPORTS_H
#define _EXPORTS_H

#include "_global.h"

#ifdef __cplusplus
extern "C"
{
#endif

DLL_EXPORT duint _dbg_memfindbaseaddr(duint addr, duint* size);
DLL_EXPORT bool _dbg_memread(duint addr, unsigned char* dest, duint size, duint* read);
DLL_EXPORT bool _dbg_memmap(MEMMAP* memmap);
DLL_EXPORT void _dbg_dbgexitsignal();
DLL_EXPORT bool _dbg_valfromstring(const char* string, duint* value);
DLL_EXPORT bool _dbg_isdebugging();
DLL_EXPORT bool _dbg_isjumpgoingtoexecute(duint addr);
DLL_EXPORT bool _dbg_addrinfoget(duint addr, SEGMENTREG segment, ADDRINFO* addrinfo);
DLL_EXPORT bool _dbg_addrinfoset(duint addr, ADDRINFO* addrinfo);
DLL_EXPORT BPXTYPE _dbg_bpgettypeat(duint addr);
DLL_EXPORT bool _dbg_getregdump(REGDUMP* regdump);
DLL_EXPORT bool _dbg_valtostring(const char* string, duint* value);

#ifdef __cplusplus
}
#endif

#endif // _EXPORTS_H

#ifndef _X64_DBG_H
#define _X64_DBG_H

#include <windows.h>
#include "_global.h"

#ifdef __cplusplus
extern "C"
{
#endif

DLL_EXPORT const char* _dbg_dbginit();
DLL_EXPORT bool _dbg_dbgcmdexec(const char* cmd);
DLL_EXPORT bool _dbg_dbgcmddirectexec(const char* cmd);
DLL_EXPORT void _dbg_dbgexitsignal();

#ifdef __cplusplus
}
#endif

COMMAND* dbggetcommandlist();

#endif // _X64_DBG_H

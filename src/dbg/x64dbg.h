#pragma once

#include "_global.h"

#ifdef __cplusplus
extern "C"
{
#endif

DLL_EXPORT const char* _dbg_dbginit(bool blocking);
DLL_EXPORT bool _dbg_dbgcmdexecasync(const char* cmd);
DLL_EXPORT bool _dbg_dbgcmddirectexec(const char* cmd);
DLL_EXPORT void _dbg_dbgexitsignal();

#ifdef __cplusplus
}
#endif

bool dbgisstopped();

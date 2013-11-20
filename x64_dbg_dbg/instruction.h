#ifndef _INSTRUCTIONS_H
#define _INSTRUCTIONS_H

#include "_global.h"
#include "command.h"

//functions
CMDRESULT cbBadCmd(const char* cmd);
CMDRESULT cbInstrVar(const char* cmd);
CMDRESULT cbInstrVarDel(const char* cmd);
CMDRESULT cbInstrMov(const char* cmd);
CMDRESULT cbInstrVarList(const char* cmd);
CMDRESULT cbInstrChd(const char* cmd);
CMDRESULT cbInstrCmt(const char* cmd);
CMDRESULT cbInstrCmtdel(const char* cmd);
CMDRESULT cbInstrLbl(const char* cmd);
CMDRESULT cbInstrLbldel(const char* cmd);
CMDRESULT cbInstrBookmarkSet(const char* cmd);
CMDRESULT cbInstrBookmarkDel(const char* cmd);
CMDRESULT cbLoaddb(const char* cmd);
CMDRESULT cbSavedb(const char* cmd);

#endif // _INSTRUCTIONS_H

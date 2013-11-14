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

#endif // _INSTRUCTIONS_H

#ifndef _INSTRUCTIONS_H
#define _INSTRUCTIONS_H

#include "_global.h"
#include "command.h"

//functions
CMDRESULT cbBadCmd(int argc, char* argv[]);
CMDRESULT cbInstrVar(int argc, char* argv[]);
CMDRESULT cbInstrVarDel(int argc, char* argv[]);
CMDRESULT cbInstrMov(int argc, char* argv[]);
CMDRESULT cbInstrVarList(int argc, char* argv[]);
CMDRESULT cbInstrChd(int argc, char* argv[]);
CMDRESULT cbInstrCmt(int argc, char* argv[]);
CMDRESULT cbInstrCmtdel(int argc, char* argv[]);
CMDRESULT cbInstrLbl(int argc, char* argv[]);
CMDRESULT cbInstrLbldel(int argc, char* argv[]);
CMDRESULT cbInstrBookmarkSet(int argc, char* argv[]);
CMDRESULT cbInstrBookmarkDel(int argc, char* argv[]);
CMDRESULT cbLoaddb(int argc, char* argv[]);
CMDRESULT cbSavedb(int argc, char* argv[]);
CMDRESULT cbAssemble(int argc, char* argv[]);
CMDRESULT cbFunctionAdd(int argc, char* argv[]);
CMDRESULT cbFunctionDel(int argc, char* argv[]);
CMDRESULT cbInstrCmp(int argc, char* argv[]);

#endif // _INSTRUCTIONS_H

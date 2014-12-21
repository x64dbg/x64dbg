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
CMDRESULT cbInstrGpa(int argc, char* argv[]);
CMDRESULT cbInstrAdd(int argc, char* argv[]);
CMDRESULT cbInstrAnd(int argc, char* argv[]);
CMDRESULT cbInstrDec(int argc, char* argv[]);
CMDRESULT cbInstrDiv(int argc, char* argv[]);
CMDRESULT cbInstrInc(int argc, char* argv[]);
CMDRESULT cbInstrMul(int argc, char* argv[]);
CMDRESULT cbInstrNeg(int argc, char* argv[]);
CMDRESULT cbInstrNot(int argc, char* argv[]);
CMDRESULT cbInstrOr(int argc, char* argv[]);
CMDRESULT cbInstrRol(int argc, char* argv[]);
CMDRESULT cbInstrRor(int argc, char* argv[]);
CMDRESULT cbInstrShl(int argc, char* argv[]);
CMDRESULT cbInstrShr(int argc, char* argv[]);
CMDRESULT cbInstrSub(int argc, char* argv[]);
CMDRESULT cbInstrTest(int argc, char* argv[]);
CMDRESULT cbInstrXor(int argc, char* argv[]);

CMDRESULT cbInstrRefinit(int argc, char* argv[]);
CMDRESULT cbInstrRefadd(int argc, char* argv[]);
CMDRESULT cbInstrRefFind(int argc, char* argv[]);
CMDRESULT cbInstrRefStr(int argc, char* argv[]);
CMDRESULT cbInstrRefFindRange(int argc, char* argv[]);

CMDRESULT cbInstrSetstr(int argc, char* argv[]);
CMDRESULT cbInstrGetstr(int argc, char* argv[]);
CMDRESULT cbInstrCopystr(int argc, char* argv[]);

CMDRESULT cbInstrFind(int argc, char* argv[]);
CMDRESULT cbInstrFindAll(int argc, char* argv[]);
CMDRESULT cbInstrModCallFind(int argc, char* argv[]);
CMDRESULT cbInstrCommentList(int argc, char* argv[]);
CMDRESULT cbInstrLabelList(int argc, char* argv[]);
CMDRESULT cbInstrBookmarkList(int argc, char* argv[]);
CMDRESULT cbInstrFunctionList(int argc, char* argv[]);
CMDRESULT cbInstrLoopList(int argc, char* argv[]);
CMDRESULT cbInstrSleep(int argc, char* argv[]);
CMDRESULT cbInstrFindAsm(int argc, char* argv[]);

#endif // _INSTRUCTIONS_H

#ifndef _INSTRUCTION_H
#define _INSTRUCTION_H

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
CMDRESULT cbInstrLoaddb(int argc, char* argv[]);
CMDRESULT cbInstrSavedb(int argc, char* argv[]);
CMDRESULT cbInstrAssemble(int argc, char* argv[]);
CMDRESULT cbInstrFunctionAdd(int argc, char* argv[]);
CMDRESULT cbInstrFunctionDel(int argc, char* argv[]);
CMDRESULT cbInstrFunctionClear(int argc, char* argv[]);
CMDRESULT cbInstrArgumentAdd(int argc, char* argv[]);
CMDRESULT cbInstrArgumentDel(int argc, char* argv[]);
CMDRESULT cbInstrArgumentClear(int argc, char* argv[]);

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
CMDRESULT cbInstrSar(int argc, char* argv[]);
CMDRESULT cbInstrSub(int argc, char* argv[]);
CMDRESULT cbInstrTest(int argc, char* argv[]);
CMDRESULT cbInstrXor(int argc, char* argv[]);
CMDRESULT cbInstrPush(int argc, char* argv[]);
CMDRESULT cbInstrPop(int argc, char* argv[]);
CMDRESULT cbInstrBswap(int argc, char* argv[]);

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
CMDRESULT cbInstrFindMemAll(int argc, char* argv[]);
CMDRESULT cbInstrModCallFind(int argc, char* argv[]);
CMDRESULT cbInstrCommentList(int argc, char* argv[]);
CMDRESULT cbInstrLabelList(int argc, char* argv[]);
CMDRESULT cbInstrBookmarkList(int argc, char* argv[]);
CMDRESULT cbInstrFunctionList(int argc, char* argv[]);
CMDRESULT cbInstrArgumentList(int argc, char* argv[]);
CMDRESULT cbInstrLoopList(int argc, char* argv[]);
CMDRESULT cbInstrSleep(int argc, char* argv[]);
CMDRESULT cbInstrFindAsm(int argc, char* argv[]);
CMDRESULT cbInstrYara(int argc, char* argv[]);
CMDRESULT cbInstrYaramod(int argc, char* argv[]);
CMDRESULT cbInstrLog(int argc, char* argv[]);

CMDRESULT cbInstrCapstone(int argc, char* argv[]);
CMDRESULT cbInstrAnalyseNukem(int argc, char* argv[]);
CMDRESULT cbInstrAnalyse(int argc, char* argv[]);
CMDRESULT cbInstrAnalyseadv(int argc, char* argv[]);
CMDRESULT cbInstrAnalrecur(int argc, char* argv[]);
CMDRESULT cbInstrAnalxrefs(int argc, char* argv[]);
CMDRESULT cbInstrVisualize(int argc, char* argv[]);
CMDRESULT cbInstrMeminfo(int argc, char* argv[]);
CMDRESULT cbInstrCfanalyse(int argc, char* argv[]);
CMDRESULT cbInstrExanalyse(int argc, char* argv[]);
CMDRESULT cbInstrVirtualmod(int argc, char* argv[]);
CMDRESULT cbInstrSetMaxFindResult(int argc, char* argv[]);
CMDRESULT cbInstrSavedata(int argc, char* argv[]);
CMDRESULT cbInstrMnemonichelp(int argc, char* argv[]);
CMDRESULT cbInstrMnemonicbrief(int argc, char* argv[]);

CMDRESULT cbInstrDataUnknown(int argc, char* argv[]);
CMDRESULT cbInstrDataByte(int argc, char* argv[]);
CMDRESULT cbInstrDataWord(int argc, char* argv[]);
CMDRESULT cbInstrDataDword(int argc, char* argv[]);
CMDRESULT cbInstrDataFword(int argc, char* argv[]);
CMDRESULT cbInstrDataQword(int argc, char* argv[]);
CMDRESULT cbInstrDataTbyte(int argc, char* argv[]);
CMDRESULT cbInstrDataOword(int argc, char* argv[]);
CMDRESULT cbInstrDataMmword(int argc, char* argv[]);
CMDRESULT cbInstrDataXmmword(int argc, char* argv[]);
CMDRESULT cbInstrDataYmmword(int argc, char* argv[]);
CMDRESULT cbInstrDataFloat(int argc, char* argv[]);
CMDRESULT cbInstrDataDouble(int argc, char* argv[]);
CMDRESULT cbInstrDataLongdouble(int argc, char* argv[]);
CMDRESULT cbInstrDataAscii(int argc, char* argv[]);
CMDRESULT cbInstrDataUnicode(int argc, char* argv[]);
CMDRESULT cbInstrDataCode(int argc, char* argv[]);
CMDRESULT cbInstrDataJunk(int argc, char* argv[]);
CMDRESULT cbInstrDataMiddle(int argc, char* argv[]);

CMDRESULT cbGetPrivilegeState(int argc, char* argv[]);
CMDRESULT cbEnablePrivilege(int argc, char* argv[]);
CMDRESULT cbDisablePrivilege(int argc, char* argv[]);
CMDRESULT cbHandleClose(int argc, char* argv[]);
CMDRESULT cbInstrBriefcheck(int argc, char* argv[]);

CMDRESULT cbInstrDisableGuiUpdate(int argc, char* argv[]);
CMDRESULT cbInstrEnableGuiUpdate(int argc, char* argv[]);
CMDRESULT cbInstrExhandlers(int argc, char* argv[]);
CMDRESULT cbInstrInstrUndo(int argc, char* argv[]);
CMDRESULT cbInstrExinfo(int argc, char* argv[]);
CMDRESULT cbInstrGraph(int argc, char* argv[]);
CMDRESULT cbInstrDisableLog(int argc, char* argv[]);
CMDRESULT cbInstrEnableLog(int argc, char* argv[]);
CMDRESULT cbInstrAddFavTool(int argc, char* argv[]);
CMDRESULT cbInstrAddFavCmd(int argc, char* argv[]);
CMDRESULT cbInstrSetFavToolShortcut(int argc, char* argv[]);
CMDRESULT cbInstrFoldDisassembly(int argc, char* argv[]);
CMDRESULT cbInstrImageinfo(int argc, char* argv[]);
CMDRESULT cbInstrTraceexecute(int argc, char* argv[]);
CMDRESULT cbInstrGetTickCount(int argc, char* argv[]);
CMDRESULT cbPluginUnload(int argc, char* argv[]);
CMDRESULT cbPluginLoad(int argc, char* argv[]);

#endif // _INSTRUCTION_H

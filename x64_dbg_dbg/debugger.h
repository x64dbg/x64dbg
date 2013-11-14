#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "_global.h"
#include "TitanEngine\TitanEngine.h"
#include "command.h"
#include "breakpoint.h"

//structures
struct INIT_STRUCT
{
    char* exe;
    char* commandline;
    char* currentfolder;
};

//functions
void dbgdisablebpx();
void dbgenablebpx();
bool dbgisrunning();
void DebugUpdateGui(uint disasm_addr);
//callbacks
CMDRESULT cbDebugInit(const char* cmd);
CMDRESULT cbStopDebug(const char* cmd);
CMDRESULT cbDebugRun(const char* cmd);
CMDRESULT cbDebugSetBPXOptions(const char* cmd);
CMDRESULT cbDebugSetBPX(const char* cmd);
CMDRESULT cbDebugEnableBPX(const char* cmd);
CMDRESULT cbDebugDisableBPX(const char* cmd);
CMDRESULT cbDebugToggleBPX(const char* cmd);
CMDRESULT cbDebugDeleteBPX(const char* cmd);
CMDRESULT cbDebugBplist(const char* cmd);
CMDRESULT cbDebugStepInto(const char* cmd);
CMDRESULT cbDebugStepOver(const char* cmd);
CMDRESULT cbDebugSingleStep(const char* cmd);
CMDRESULT cbDebugHide(const char* cmd);
CMDRESULT cbDebugDisasm(const char* cmd);
CMDRESULT cbDebugMemoryBpx(const char* cmd);
CMDRESULT cbDebugRtr(const char* cmd);
CMDRESULT cbDebugSetHardwareBreakpoint(const char* cmd);
CMDRESULT cbDebugAlloc(const char* cmd);
CMDRESULT cbDebugFree(const char* cmd);
CMDRESULT cbDebugMemset(const char* cmd);
CMDRESULT cbBenchmark(const char* cmd);
CMDRESULT cbDebugPause(const char* cmd);
CMDRESULT cbMemWrite(const char* cmd);
CMDRESULT cbStartScylla(const char* cmd);

//variables
extern PROCESS_INFORMATION* fdProcessInfo;
extern BREAKPOINT* bplist;

#endif // _DEBUGGER_H

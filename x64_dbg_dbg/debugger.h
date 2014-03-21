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

struct ExceptionRange
{
    unsigned int start;
    unsigned int end;
};

//functions
void dbgdisablebpx();
void dbgenablebpx();
bool dbgisrunning();
void DebugUpdateGui(uint disasm_addr, bool stack);
void dbgsetskipexceptions(bool skip);
void dbgclearignoredexceptions();
void dbgaddignoredexception(ExceptionRange range);
bool dbgisignoredexception(unsigned int exception);
//callbacks
CMDRESULT cbDebugInit(int argc, char* argv[]);
CMDRESULT cbStopDebug(int argc, char* argv[]);
CMDRESULT cbDebugRun(int argc, char* argv[]);
CMDRESULT cbDebugErun(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXOptions(int argc, char* argv[]);
CMDRESULT cbDebugSetBPX(int argc, char* argv[]);
CMDRESULT cbDebugDeleteBPX(int argc, char* argv[]);
CMDRESULT cbDebugEnableBPX(int argc, char* argv[]);
CMDRESULT cbDebugDisableBPX(int argc, char* argv[]);
CMDRESULT cbDebugBplist(int argc, char* argv[]);
CMDRESULT cbDebugStepInto(int argc, char* argv[]);
CMDRESULT cbDebugeStepInto(int argc, char* argv[]);
CMDRESULT cbDebugStepOver(int argc, char* argv[]);
CMDRESULT cbDebugeStepOver(int argc, char* argv[]);
CMDRESULT cbDebugSingleStep(int argc, char* argv[]);
CMDRESULT cbDebugeSingleStep(int argc, char* argv[]);
CMDRESULT cbDebugHide(int argc, char* argv[]);
CMDRESULT cbDebugDisasm(int argc, char* argv[]);
CMDRESULT cbDebugSetMemoryBpx(int argc, char* argv[]);
CMDRESULT cbDebugRtr(int argc, char* argv[]);
CMDRESULT cbDebugeRtr(int argc, char* argv[]);
CMDRESULT cbDebugSetHardwareBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugAlloc(int argc, char* argv[]);
CMDRESULT cbDebugFree(int argc, char* argv[]);
CMDRESULT cbDebugMemset(int argc, char* argv[]);
CMDRESULT cbBenchmark(int argc, char* argv[]);
CMDRESULT cbDebugPause(int argc, char* argv[]);
CMDRESULT cbMemWrite(int argc, char* argv[]);
CMDRESULT cbStartScylla(int argc, char* argv[]);
CMDRESULT cbDebugDeleteHardwareBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugDeleteMemoryBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugAttach(int argc, char* argv[]);
CMDRESULT cbDebugDetach(int argc, char* argv[]);
CMDRESULT cbDebugDump(int argc, char* argv[]);
CMDRESULT cbDebugStackDump(int argc, char* argv[]);

//variables
extern PROCESS_INFORMATION* fdProcessInfo;
extern BREAKPOINT* bplist;

#endif // _DEBUGGER_H

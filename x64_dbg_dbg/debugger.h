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

#pragma pack(push,8)
typedef struct _THREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

//functions
SIZE_T dbggetprivateusage(HANDLE hProcess, bool update = false);
void dbginit();
uint dbgdebuggedbase();
void dbgdisablebpx();
void dbgenablebpx();
bool dbgisrunning();
bool dbgisdll();
void DebugUpdateGui(uint disasm_addr, bool stack);
void dbgsetskipexceptions(bool skip);
void dbgsetstepping(bool stepping);
void dbgsetispausedbyuser(bool b);
void dbgsetisdetachedbyuser(bool b);
void dbgclearignoredexceptions();
void dbgaddignoredexception(ExceptionRange range);
bool dbgisignoredexception(unsigned int exception);
bool dbgcmdnew(const char* name, CBCOMMAND cbCommand, bool debugonly);
bool dbgcmddel(const char* name);
bool dbggetjit(char** jit_entry_out, arch arch_in, arch* arch_out);
bool dbgsetjit(char* jit_cmd, arch arch_in, arch* arch_out);

void cbStep();
void cbRtrStep();
void cbSystemBreakpoint(void* ExceptionData);
void cbMemoryBreakpoint(void* ExceptionAddress);
void cbHardwareBreakpoint(void* ExceptionAddress);
void cbUserBreakpoint();
void cbLibrarianBreakpoint(void* lpData);
DWORD WINAPI threadDebugLoop(void* lpParameter);
bool cbDeleteAllBreakpoints(const BREAKPOINT* bp);
bool cbEnableAllBreakpoints(const BREAKPOINT* bp);
bool cbDisableAllBreakpoints(const BREAKPOINT* bp);
bool cbEnableAllHardwareBreakpoints(const BREAKPOINT* bp);
bool cbDisableAllHardwareBreakpoints(const BREAKPOINT* bp);
bool cbEnableAllMemoryBreakpoints(const BREAKPOINT* bp);
bool cbDisableAllMemoryBreakpoints(const BREAKPOINT* bp);
bool cbBreakpointList(const BREAKPOINT* bp);
bool cbDeleteAllMemoryBreakpoints(const BREAKPOINT* bp);
bool cbDeleteAllHardwareBreakpoints(const BREAKPOINT* bp);
DWORD WINAPI threadAttachLoop(void* lpParameter);
void cbDetach();

//variables
extern PROCESS_INFORMATION* fdProcessInfo;
extern HANDLE hActiveThread;
extern char szFileName[MAX_PATH];
extern char szSymbolCachePath[MAX_PATH];

#endif // _DEBUGGER_H

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "_global.h"
#include "TitanEngine\TitanEngine.h"
#include "command.h"
#include "breakpoint.h"
#include "undocumented.h"
#include "expressionparser.h"
#include "value.h"
#include "_plugins.h"

//structures
struct INIT_STRUCT
{
    char* exe;
    char* commandline;
    char* currentfolder;
};

typedef enum
{
    CMDL_ERR_READ_PEBBASE = 0,
    CMDL_ERR_READ_PROCPARM_PTR,
    CMDL_ERR_READ_PROCPARM_CMDLINE,
    CMDL_ERR_CONVERTUNICODE,
    CMDL_ERR_ALLOC,
    CMDL_ERR_GET_PEB,
    CMDL_ERR_READ_GETCOMMANDLINEBASE,
    CMDL_ERR_CHECK_GETCOMMANDLINESTORED,
    CMDL_ERR_WRITE_GETCOMMANDLINESTORED,
    CMDL_ERR_GET_GETCOMMANDLINE,
    CMDL_ERR_ALLOC_UNICODEANSI_COMMANDLINE,
    CMDL_ERR_WRITE_ANSI_COMMANDLINE,
    CMDL_ERR_WRITE_UNICODE_COMMANDLINE,
    CMDL_ERR_WRITE_PEBUNICODE_COMMANDLINE

} cmdline_error_type_t;

typedef struct
{
    cmdline_error_type_t type;
    duint addr;
} cmdline_error_t;

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
void dbginit();
void dbgstop();
duint dbgdebuggedbase();
duint dbggettimewastedcounter();
bool dbgisrunning();
bool dbgisdll();
void dbgsetattachevent(HANDLE handle);
void DebugUpdateGui(duint disasm_addr, bool stack);
void DebugUpdateStack(duint dumpAddr, duint csp, bool forceDump = false);
void dbgsetskipexceptions(bool skip);
void dbgsetstepping(bool stepping);
void dbgsetispausedbyuser(bool b);
void dbgsetisdetachedbyuser(bool b);
void dbgsetfreezestack(bool freeze);
void dbgclearignoredexceptions();
void dbgaddignoredexception(ExceptionRange range);
bool dbgisignoredexception(unsigned int exception);
bool dbgcmdnew(const char* name, CBCOMMAND cbCommand, bool debugonly);
bool dbgcmddel(const char* name);
bool dbglistprocesses(std::vector<PROCESSENTRY32>* list);
bool dbgsetcmdline(const char* cmd_line, cmdline_error_t* cmd_line_error);
bool dbggetcmdline(char** cmd_line, cmdline_error_t* cmd_line_error);
void dbgstartscriptthread(CBPLUGINSCRIPT cbScript);
duint dbggetdebuggedbase();
duint dbggetdbgevents();
bool dbgsettracecondition(String expression, duint maxCount);
bool dbgtraceactive();

void cbStep();
void cbRtrStep();
void cbPauseBreakpoint();
void cbSystemBreakpoint(void* ExceptionData);
void cbMemoryBreakpoint(void* ExceptionAddress);
void cbHardwareBreakpoint(void* ExceptionAddress);
void cbUserBreakpoint();
void cbDebugLoadLibBPX();
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
void cbTOCNDStep();
void cbTICNDStep();
void cbTIBTStep();
void cbTOBTStep();
void cbTIITStep();
void cbTOITStep();
void cbRunToUserCodeBreakpoint(void* ExceptionAddress);
DWORD WINAPI threadAttachLoop(void* lpParameter);
void cbDetach();
bool cbSetModuleBreakpoints(const BREAKPOINT* bp);

//variables
extern PROCESS_INFORMATION* fdProcessInfo;
extern HANDLE hActiveThread;
extern HANDLE hProcessToken;
extern char szFileName[MAX_PATH];
extern char szSymbolCachePath[MAX_PATH];
extern bool bUndecorateSymbolNames;
extern bool bEnableSourceDebugging;

#endif // _DEBUGGER_H

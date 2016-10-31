#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "_global.h"
#include "TitanEngine\TitanEngine.h"
#include "command.h"
#include "breakpoint.h"
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

typedef enum
{
    NO_QOUTES = 0,
    QOUTES_AROUND_EXE,
    QOUTES_AT_BEGIN_AND_END,
    NO_CLOSE_QUOTE_FOUND

} cmdline_qoutes_placement_t_enum;

typedef struct
{
    cmdline_qoutes_placement_t_enum posEnum;
    size_t firstPos;
    size_t secondPos;
} cmdline_qoutes_placement_t;

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
void DebugUpdateGuiAsync(duint disasm_addr, bool stack);
void DebugUpdateGuiSetStateAsync(duint disasm_addr, bool stack, DBGSTATE state = paused);
void DebugUpdateBreakpointsViewAsync();
void DebugUpdateStack(duint dumpAddr, duint csp, bool forceDump = false);
void GuiSetDebugStateAsync(DBGSTATE state);
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
bool dbglistprocesses(std::vector<PROCESSENTRY32>* infoList, std::vector<std::string>* commandList);
bool dbgsetcmdline(const char* cmd_line, cmdline_error_t* cmd_line_error);
bool dbggetcmdline(char** cmd_line, cmdline_error_t* cmd_line_error, HANDLE hProcess = NULL);
cmdline_qoutes_placement_t getqoutesplacement(const char* cmdline);
void dbgstartscriptthread(CBPLUGINSCRIPT cbScript);
duint dbggetdebuggedbase();
duint dbggetdbgevents();
bool dbgsettracecondition(const String & expression, duint maxCount);
bool dbgsettracelog(const String & expression, const String & text);
bool dbgsettracecmd(const String & expression, const String & text);
bool dbgtraceactive();
void dbgsetdebuggeeinitscript(const char* fileName);
const char* dbggetdebuggeeinitscript();
void dbgsetforeground();

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
void cbTraceOverConditionalStep();
void cbTraceIntoConditionalStep();
void cbTraceIntoBeyondTraceRecordStep();
void cbTraceOverBeyondTraceRecordStep();
void cbTraceIntoIntoTraceRecordStep();
void cbTraceOverIntoTraceRecordStep();
void cbRunToUserCodeBreakpoint(void* ExceptionAddress);
DWORD WINAPI threadAttachLoop(void* lpParameter);
void cbDetach();
bool cbSetModuleBreakpoints(const BREAKPOINT* bp);
EXCEPTION_DEBUG_INFO getLastExceptionInfo();

//variables
extern PROCESS_INFORMATION* fdProcessInfo;
extern HANDLE hActiveThread;
extern HANDLE hProcessToken;
extern char szProgramDir[MAX_PATH];
extern char szFileName[MAX_PATH];
extern char szSymbolCachePath[MAX_PATH];
extern bool bUndecorateSymbolNames;
extern bool bEnableSourceDebugging;
extern bool bTraceRecordEnabledDuringTrace;
extern bool bSkipInt3Stepping;
extern bool bIgnoreInconsistentBreakpoints;
extern bool bNoForegroundWindow;

#endif // _DEBUGGER_H

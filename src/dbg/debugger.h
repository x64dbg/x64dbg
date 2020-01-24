#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "_global.h"
#include "TitanEngine/TitanEngine.h"
#include "command.h"
#include "breakpoint.h"
#include "_plugins.h"
#include "commandline.h"
#include <tlhelp32.h>
#include <psapi.h>

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
void dbginit();
void dbgstop();
duint dbgdebuggedbase();
duint dbggettimewastedcounter();
bool dbgisrunning();
bool dbgisdll();
void dbgsetattachevent(HANDLE handle);
void dbgsetresumetid(duint tid);
void DebugUpdateGui(duint disasm_addr, bool stack);
void DebugUpdateGuiAsync(duint disasm_addr, bool stack);
void DebugUpdateTitleAsync(duint disasm_addr, bool analyzeThreadSwitch);
void DebugUpdateGuiSetStateAsync(duint disasm_addr, bool stack, DBGSTATE state = paused);
void DebugUpdateBreakpointsViewAsync();
void DebugUpdateStack(duint dumpAddr, duint csp, bool forceDump = false);
void DebugRemoveBreakpoints();
void DebugSetBreakpoints();
void GuiSetDebugStateAsync(DBGSTATE state);
void dbgsetskipexceptions(bool skip);
void dbgsetsteprepeat(bool steppingIn, duint repeat);
void dbgsetisdetachedbyuser(bool b);
void dbgsetfreezestack(bool freeze);
void dbgclearignoredexceptions();
void dbgaddignoredexception(ExceptionRange range);
bool dbgisignoredexception(unsigned int exception);
bool dbgcmdnew(const char* name, CBCOMMAND cbCommand, bool debugonly);
bool dbgcmddel(const char* name);
bool dbglistprocesses(std::vector<PROCESSENTRY32>* infoList, std::vector<std::string>* commandList, std::vector<std::string>* winTextList);
bool dbgsetcmdline(const char* cmd_line, cmdline_error_t* cmd_line_error);
bool dbggetcmdline(char** cmd_line, cmdline_error_t* cmd_line_error, HANDLE hProcess = NULL);
cmdline_qoutes_placement_t getqoutesplacement(const char* cmdline);
void dbgstartscriptthread(CBPLUGINSCRIPT cbScript);
duint dbggetdbgevents();
bool dbgsettracecondition(const String & expression, duint maxCount);
bool dbgsettracelog(const String & expression, const String & text);
bool dbgsettracecmd(const String & expression, const String & text);
bool dbgsettraceswitchcondition(const String & expression);
bool dbgtraceactive();
void dbgforcebreaktrace();
bool dbgsettracelogfile(const char* fileName);
void dbgsetdebuggeeinitscript(const char* fileName);
const char* dbggetdebuggeeinitscript();
void dbgsetforeground();
bool dbggetwintext(std::vector<std::string>* winTextList, const DWORD dwProcessId);
void dbgtracebrowserneedsupdate();
bool dbgsetdllbreakpoint(const char* mod, DWORD type, bool singleshoot);
bool dbgdeletedllbreakpoint(const char* mod, DWORD type);

void cbStep();
void cbRtrStep();
void cbPauseBreakpoint();
void cbSystemBreakpoint(void* ExceptionData);
void cbMemoryBreakpoint(void* ExceptionAddress);
void cbHardwareBreakpoint(void* ExceptionAddress);
void cbUserBreakpoint();
void cbDebugLoadLibBPX();
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
EXCEPTION_DEBUG_INFO & getLastExceptionInfo();
bool dbgrestartadmin();
void StepIntoWow64(LPVOID traceCallBack);
void StepOverWrapper(LPVOID traceCallBack);
bool dbgisdepenabled();
BOOL CALLBACK chkWindowPidCallback(HWND hWnd, LPARAM lParam);
BOOL ismainwindow(HWND handle);

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
extern bool bIgnoreInconsistentBreakpoints;
extern bool bNoForegroundWindow;
extern bool bVerboseExceptionLogging;
extern bool bNoWow64SingleStepWorkaround;
extern bool bForceLoadSymbols;
extern duint maxSkipExceptionCount;
extern HANDLE mProcHandle;
extern HANDLE mForegroundHandle;
extern duint mRtrPreviousCSP;
extern HANDLE hDebugLoopThread;

#endif // _DEBUGGER_H

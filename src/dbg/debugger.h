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

//enums
enum class ExceptionBreakOn
{
    FirstChance,
    SecondChance,
    DoNotBreak
};

enum class ExceptionHandledBy
{
    Debugger,
    Debuggee
};

//structures
struct INIT_STRUCT
{
    HANDLE event = nullptr;
    char* exe = nullptr;
    char* commandline = nullptr;
    char* currentfolder = nullptr;
    DWORD pid = 0;
    bool attach = false;
};

struct ExceptionRange
{
    unsigned int start;
    unsigned int end;
};

struct ExceptionFilter
{
    ExceptionRange range;
    ExceptionBreakOn breakOn;
    bool logException;
    ExceptionHandledBy handledBy;
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
void dbgsetfreezestack(bool freeze);
void dbgclearexceptionfilters();
void dbgaddexceptionfilter(ExceptionFilter filter);
const ExceptionFilter & dbggetexceptionfilter(unsigned int exception);
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
void dbgsetdebugflags(DWORD flags);
void dbgcreatedebugthread(INIT_STRUCT* init);
String formatpidtid(DWORD pidtid);

void cbStep();
void cbRtrStep();
void cbPauseBreakpoint();
void cbSystemBreakpoint(void* ExceptionData);
void cbMemoryBreakpoint(void* ExceptionAddress);
void cbHardwareBreakpoint(void* ExceptionAddress);
void cbUserBreakpoint();
void cbDebugLoadLibBPX();
void cbTraceOverConditionalStep();
void cbTraceIntoConditionalStep();
void cbTraceIntoBeyondTraceRecordStep();
void cbTraceOverBeyondTraceRecordStep();
void cbTraceIntoIntoTraceRecordStep();
void cbTraceOverIntoTraceRecordStep();
void cbRunToUserCodeBreakpoint(void* ExceptionAddress);
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
extern char szDebuggeePath[MAX_PATH];
extern char szDllLoaderPath[MAX_PATH];
extern char szSymbolCachePath[MAX_PATH];
extern bool bUndecorateSymbolNames;
extern bool bEnableSourceDebugging;
extern bool bTraceRecordEnabledDuringTrace;
extern bool bSkipInt3Stepping;
extern bool bIgnoreInconsistentBreakpoints;
extern bool bNoForegroundWindow;
extern bool bVerboseExceptionLogging;
extern bool bNoWow64SingleStepWorkaround;
extern bool bForceLoadSymbols;
extern bool bNewStringAlgorithm;
extern bool bPidTidInHex;
extern duint maxSkipExceptionCount;
extern HANDLE mProcHandle;
extern HANDLE mForegroundHandle;
extern duint mRtrPreviousCSP;
extern HANDLE hDebugLoopThread;

#endif // _DEBUGGER_H

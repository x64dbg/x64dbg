#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "_global.h"
#include "TitanEngine\TitanEngine.h"
#include "command.h"
#include "breakpoint.h"
#include "undocumented.h"
#include "value.h"

#define ATTACH_CMD_LINE "\" -a %ld -e %ld"
#define JIT_ENTRY_DEF_SIZE (MAX_PATH + sizeof(ATTACH_CMD_LINE) + 2)
#define JIT_ENTRY_MAX_SIZE 512
#define JIT_REG_KEY TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug")
#define MS_VC_EXCEPTION 0x406D1388

typedef enum
{
    ERROR_RW = 0,
    ERROR_RW_FILE_NOT_FOUND,
    ERROR_RW_NOTWOW64,
    ERROR_RW_NOTADMIN
} readwritejitkey_error_t;

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
    uint addr;
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
SIZE_T dbggetprivateusage(HANDLE hProcess, bool update = false);
void dbginit();
uint dbgdebuggedbase();
void dbgdisablebpx();
void dbgenablebpx();
bool dbgisrunning();
bool dbgisdll();
void dbgsetattachevent(HANDLE handle);
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
bool dbggetjit(char jit_entry[JIT_ENTRY_MAX_SIZE], arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out);
bool dbgsetjit(char* jit_cmd, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out);
bool dbggetjitauto(bool* auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out);
bool dbgsetjitauto(bool auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out);
bool dbggetdefjit(char* jit_entry);
bool dbglistprocesses(std::vector<PROCESSENTRY32>* list);
bool IsProcessElevated();
bool dbgsetcmdline(const char* cmd_line, cmdline_error_t* cmd_line_error);
bool dbggetcmdline(char** cmd_line, cmdline_error_t* cmd_line_error);
bool dbggetpagerights(uint addr, char* rights);
bool dbgsetpagerights(uint addr, const char* rights_string);
bool dbgpagerightstostring(DWORD protect, char* rights);

void cbStep();
void cbRtrStep();
void cbSystemBreakpoint(void* ExceptionData);
void cbMemoryBreakpoint(void* ExceptionAddress);
void cbHardwareBreakpoint(void* ExceptionAddress);
void cbUserBreakpoint();
void cbLoadLibBPX();
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
extern bool bUndecorateSymbolNames;

#endif // _DEBUGGER_H

#ifndef _BRIDGEMAIN_H_
#define _BRIDGEMAIN_H_

#include <windows.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

//list structure (and C++ wrapper)
#include "bridgelist.h"

//default structure alignments forced
#ifdef _WIN64
#pragma pack(push, 16)
#else //x86
#pragma pack(push, 8)
#endif //_WIN64

#ifdef _WIN64
typedef unsigned long long duint;
typedef signed long long dsint;
#else
typedef unsigned long duint;
typedef signed long dsint;
#endif //_WIN64

#ifndef BRIDGE_IMPEXP
#ifdef BUILD_BRIDGE
#define BRIDGE_IMPEXP __declspec(dllexport)
#else
#define BRIDGE_IMPEXP __declspec(dllimport)
#endif //BUILD_BRIDGE
#endif //BRIDGE_IMPEXP

#ifdef __cplusplus
extern "C"
{
#endif

//Bridge defines
#define MAX_SETTING_SIZE 65536
#define DBG_VERSION 25

//Bridge functions
BRIDGE_IMPEXP const char* BridgeInit();
BRIDGE_IMPEXP const char* BridgeStart();
BRIDGE_IMPEXP void* BridgeAlloc(size_t size);
BRIDGE_IMPEXP void BridgeFree(void* ptr);
BRIDGE_IMPEXP bool BridgeSettingGet(const char* section, const char* key, char* value);
BRIDGE_IMPEXP bool BridgeSettingGetUint(const char* section, const char* key, duint* value);
BRIDGE_IMPEXP bool BridgeSettingSet(const char* section, const char* key, const char* value);
BRIDGE_IMPEXP bool BridgeSettingSetUint(const char* section, const char* key, duint value);
BRIDGE_IMPEXP bool BridgeSettingFlush();
BRIDGE_IMPEXP bool BridgeSettingRead(int* errorLine);
BRIDGE_IMPEXP int BridgeGetDbgVersion();

//Debugger defines
#define MAX_LABEL_SIZE 2048
#define MAX_COMMENT_SIZE 512
#define MAX_MODULE_SIZE 256
#define MAX_BREAKPOINT_SIZE 256
#define MAX_SCRIPT_LINE_SIZE 2048
#define MAX_THREAD_NAME_SIZE 256
#define MAX_STRING_SIZE 512
#define MAX_ERROR_SIZE 512
#define RIGHTS_STRING_SIZE (sizeof("ERWCG") + 1)
#define MAX_SECTION_SIZE 10

#define TYPE_VALUE 1
#define TYPE_MEMORY 2
#define TYPE_ADDR 4
#define MAX_MNEMONIC_SIZE 64
#define PAGE_SIZE 0x1000

//Debugger enums
typedef enum
{
    initialized,
    paused,
    running,
    stopped
} DBGSTATE;

typedef enum
{
    SEG_DEFAULT,
    SEG_ES,
    SEG_DS,
    SEG_FS,
    SEG_GS,
    SEG_CS,
    SEG_SS
} SEGMENTREG;

typedef enum
{
    flagmodule = 1,
    flaglabel = 2,
    flagcomment = 4,
    flagbookmark = 8,
    flagfunction = 16,
    flagloop = 32
} ADDRINFOFLAGS;

typedef enum
{
    bp_none = 0,
    bp_normal = 1,
    bp_hardware = 2,
    bp_memory = 4
} BPXTYPE;

typedef enum
{
    FUNC_NONE,
    FUNC_BEGIN,
    FUNC_MIDDLE,
    FUNC_END,
    FUNC_SINGLE
} FUNCTYPE;

typedef enum
{
    LOOP_NONE,
    LOOP_BEGIN,
    LOOP_MIDDLE,
    LOOP_ENTRY,
    LOOP_END
} LOOPTYPE;

typedef enum
{
    DBG_SCRIPT_LOAD,                // param1=const char* filename,      param2=unused
    DBG_SCRIPT_UNLOAD,              // param1=unused,                    param2=unused
    DBG_SCRIPT_RUN,                 // param1=int destline,              param2=unused
    DBG_SCRIPT_STEP,                // param1=unused,                    param2=unused
    DBG_SCRIPT_BPTOGGLE,            // param1=int line,                  param2=unused
    DBG_SCRIPT_BPGET,               // param1=int line,                  param2=unused
    DBG_SCRIPT_CMDEXEC,             // param1=const char* command,       param2=unused
    DBG_SCRIPT_ABORT,               // param1=unused,                    param2=unused
    DBG_SCRIPT_GETLINETYPE,         // param1=int line,                  param2=unused
    DBG_SCRIPT_SETIP,               // param1=int line,                  param2=unused
    DBG_SCRIPT_GETBRANCHINFO,       // param1=int line,                  param2=SCRIPTBRANCH* info
    DBG_SYMBOL_ENUM,                // param1=SYMBOLCBINFO* cbInfo,      param2=unused
    DBG_ASSEMBLE_AT,                // param1=duint addr,                param2=const char* instruction
    DBG_MODBASE_FROM_NAME,          // param1=const char* modname,       param2=unused
    DBG_DISASM_AT,                  // param1=duint addr,                 param2=DISASM_INSTR* instr
    DBG_STACK_COMMENT_GET,          // param1=duint addr,                param2=STACK_COMMENT* comment
    DBG_GET_THREAD_LIST,            // param1=THREADALLINFO* list,       param2=unused
    DBG_SETTINGS_UPDATED,           // param1=unused,                    param2=unused
    DBG_DISASM_FAST_AT,             // param1=duint addr,                param2=BASIC_INSTRUCTION_INFO* basicinfo
    DBG_MENU_ENTRY_CLICKED,         // param1=int hEntry,                param2=unused
    DBG_FUNCTION_GET,               // param1=FUNCTION_LOOP_INFO* info,  param2=unused
    DBG_FUNCTION_OVERLAPS,          // param1=FUNCTION_LOOP_INFO* info,  param2=unused
    DBG_FUNCTION_ADD,               // param1=FUNCTION_LOOP_INFO* info,  param2=unused
    DBG_FUNCTION_DEL,               // param1=FUNCTION_LOOP_INFO* info,  param2=unused
    DBG_LOOP_GET,                   // param1=FUNCTION_LOOP_INFO* info,  param2=unused
    DBG_LOOP_OVERLAPS,              // param1=FUNCTION_LOOP_INFO* info,  param2=unused
    DBG_LOOP_ADD,                   // param1=FUNCTION_LOOP_INFO* info,  param2=unused
    DBG_LOOP_DEL,                   // param1=FUNCTION_LOOP_INFO* info,  param2=unused
    DBG_IS_RUN_LOCKED,              // param1=unused,                    param2=unused
    DBG_IS_BP_DISABLED,             // param1=duint addr,                param2=unused
    DBG_SET_AUTO_COMMENT_AT,        // param1=duint addr,                param2=const char* text
    DBG_DELETE_AUTO_COMMENT_RANGE,  // param1=duint start,               param2=duint end
    DBG_SET_AUTO_LABEL_AT,          // param1=duint addr,                param2=const char* text
    DBG_DELETE_AUTO_LABEL_RANGE,    // param1=duint start,               param2=duint end
    DBG_SET_AUTO_BOOKMARK_AT,       // param1=duint addr,                param2=const char* text
    DBG_DELETE_AUTO_BOOKMARK_RANGE, // param1=duint start,               param2=duint end
    DBG_SET_AUTO_FUNCTION_AT,       // param1=duint addr,                param2=const char* text
    DBG_DELETE_AUTO_FUNCTION_RANGE, // param1=duint start,               param2=duint end
    DBG_GET_STRING_AT,              // param1=duint addr,                param2=unused
    DBG_GET_FUNCTIONS,              // param1=unused,                    param2=unused
    DBG_WIN_EVENT,                  // param1=MSG* message,              param2=long* result
    DBG_WIN_EVENT_GLOBAL,           // param1=MSG* message,              param2=unused
    DBG_INITIALIZE_LOCKS,           // param1=unused,                    param2=unused
    DBG_DEINITIALIZE_LOCKS,         // param1=unused,                    param2=unused
    DBG_GET_TIME_WASTED_COUNTER     // param1=unused,                    param2=unused
} DBGMSG;

typedef enum
{
    linecommand,
    linebranch,
    linelabel,
    linecomment,
    lineempty,
} SCRIPTLINETYPE;

typedef enum
{
    scriptnobranch,
    scriptjmp,
    scriptjnejnz,
    scriptjejz,
    scriptjbjl,
    scriptjajg,
    scriptjbejle,
    scriptjaejge,
    scriptcall
} SCRIPTBRANCHTYPE;

typedef enum
{
    instr_normal,
    instr_branch,
    instr_stack
} DISASM_INSTRTYPE;

typedef enum
{
    arg_normal,
    arg_memory
} DISASM_ARGTYPE;

typedef enum
{
    str_none,
    str_ascii,
    str_unicode
} STRING_TYPE;

typedef enum
{
    _PriorityIdle = -15,
    _PriorityAboveNormal = 1,
    _PriorityBelowNormal = -1,
    _PriorityHighest = 2,
    _PriorityLowest = -2,
    _PriorityNormal = 0,
    _PriorityTimeCritical = 15,
    _PriorityUnknown = 0x7FFFFFFF
} THREADPRIORITY;

typedef enum
{
    _Executive = 0,
    _FreePage = 1,
    _PageIn = 2,
    _PoolAllocation = 3,
    _DelayExecution = 4,
    _Suspended = 5,
    _UserRequest = 6,
    _WrExecutive = 7,
    _WrFreePage = 8,
    _WrPageIn = 9,
    _WrPoolAllocation = 10,
    _WrDelayExecution = 11,
    _WrSuspended = 12,
    _WrUserRequest = 13,
    _WrEventPair = 14,
    _WrQueue = 15,
    _WrLpcReceive = 16,
    _WrLpcReply = 17,
    _WrVirtualMemory = 18,
    _WrPageOut = 19,
    _WrRendezvous = 20,
    _Spare2 = 21,
    _Spare3 = 22,
    _Spare4 = 23,
    _Spare5 = 24,
    _WrCalloutStack = 25,
    _WrKernel = 26,
    _WrResource = 27,
    _WrPushLock = 28,
    _WrMutex = 29,
    _WrQuantumEnd = 30,
    _WrDispatchInt = 31,
    _WrPreempted = 32,
    _WrYieldExecution = 33,
    _WrFastMutex = 34,
    _WrGuardedMutex = 35,
    _WrRundown = 36,
} THREADWAITREASON;

typedef enum
{
    size_byte = 1,
    size_word = 2,
    size_dword = 4,
    size_qword = 8
} MEMORY_SIZE;

//Debugger typedefs
typedef MEMORY_SIZE VALUE_SIZE;
typedef struct SYMBOLINFO_ SYMBOLINFO;
typedef struct DBGFUNCTIONS_ DBGFUNCTIONS;

typedef void (*CBSYMBOLENUM)(SYMBOLINFO* symbol, void* user);

//Debugger structs
typedef struct
{
    MEMORY_BASIC_INFORMATION mbi;
    char info[MAX_MODULE_SIZE];
} MEMPAGE;

typedef struct
{
    int count;
    MEMPAGE* page;
} MEMMAP;

typedef struct
{
    BPXTYPE type;
    duint addr;
    bool enabled;
    bool singleshoot;
    bool active;
    char name[MAX_BREAKPOINT_SIZE];
    char mod[MAX_MODULE_SIZE];
    unsigned short slot;
} BRIDGEBP;

typedef struct
{
    int count;
    BRIDGEBP* bp;
} BPMAP;

typedef struct
{
    duint start; //OUT
    duint end; //OUT
} FUNCTION;

typedef struct
{
    int depth; //IN
    duint start; //OUT
    duint end; //OUT
} LOOP;

typedef struct
{
    int flags; //ADDRINFOFLAGS (IN)
    char module[MAX_MODULE_SIZE]; //module the address is in
    char label[MAX_LABEL_SIZE];
    char comment[MAX_COMMENT_SIZE];
    bool isbookmark;
    FUNCTION function;
    LOOP loop;
} ADDRINFO;

struct SYMBOLINFO_
{
    duint addr;
    char* decoratedSymbol;
    char* undecoratedSymbol;
};

typedef struct
{
    duint base;
    char name[MAX_MODULE_SIZE];
} SYMBOLMODULEINFO;

typedef struct
{
    duint base;
    CBSYMBOLENUM cbSymbolEnum;
    void* user;
} SYMBOLCBINFO;

typedef struct
{
    bool c;
    bool p;
    bool a;
    bool z;
    bool s;
    bool t;
    bool i;
    bool d;
    bool o;
} FLAGS;

typedef struct
{
    bool FZ;
    bool PM;
    bool UM;
    bool OM;
    bool ZM;
    bool IM;
    bool DM;
    bool DAZ;
    bool PE;
    bool UE;
    bool OE;
    bool ZE;
    bool DE;
    bool IE;

    unsigned short RC;
} MXCSRFIELDS;

typedef struct
{
    bool B;
    bool C3;
    bool C2;
    bool C1;
    bool C0;
    bool IR;
    bool SF;
    bool P;
    bool U;
    bool O;
    bool Z;
    bool D;
    bool I;

    unsigned short TOP;

} X87STATUSWORDFIELDS;

typedef struct
{
    bool IC;
    bool IEM;
    bool PM;
    bool UM;
    bool OM;
    bool ZM;
    bool DM;
    bool IM;

    unsigned short RC;
    unsigned short PC;

} X87CONTROLWORDFIELDS;

typedef struct DECLSPEC_ALIGN(16) _XMMREGISTER
{
    ULONGLONG Low;
    LONGLONG High;
} XMMREGISTER;

typedef struct
{
    XMMREGISTER Low; //XMM/SSE part
    XMMREGISTER High; //AVX part
} YMMREGISTER;

typedef struct
{
    BYTE    data[10];
    int     st_value;
    int     tag;
} X87FPUREGISTER;

typedef struct
{
    WORD   ControlWord;
    WORD   StatusWord;
    WORD   TagWord;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    DWORD   Cr0NpxState;
} X87FPU;

typedef struct
{
    ULONG_PTR cax;
    ULONG_PTR ccx;
    ULONG_PTR cdx;
    ULONG_PTR cbx;
    ULONG_PTR csp;
    ULONG_PTR cbp;
    ULONG_PTR csi;
    ULONG_PTR cdi;
#ifdef _WIN64
    ULONG_PTR r8;
    ULONG_PTR r9;
    ULONG_PTR r10;
    ULONG_PTR r11;
    ULONG_PTR r12;
    ULONG_PTR r13;
    ULONG_PTR r14;
    ULONG_PTR r15;
#endif //_WIN64
    ULONG_PTR cip;
    ULONG_PTR eflags;
    unsigned short gs;
    unsigned short fs;
    unsigned short es;
    unsigned short ds;
    unsigned short cs;
    unsigned short ss;
    ULONG_PTR dr0;
    ULONG_PTR dr1;
    ULONG_PTR dr2;
    ULONG_PTR dr3;
    ULONG_PTR dr6;
    ULONG_PTR dr7;
    BYTE RegisterArea[80];
    X87FPU x87fpu;
    DWORD MxCsr;
#ifdef _WIN64
    XMMREGISTER XmmRegisters[16];
    YMMREGISTER YmmRegisters[16];
#else // x86
    XMMREGISTER XmmRegisters[8];
    YMMREGISTER YmmRegisters[8];
#endif
} REGISTERCONTEXT;

typedef struct
{
    DWORD code;
    const char* name;
} LASTERROR;

typedef struct
{
    REGISTERCONTEXT regcontext;
    FLAGS flags;
    X87FPUREGISTER x87FPURegisters[8];
    unsigned long long mmx[8];
    MXCSRFIELDS MxCsrFields;
    X87STATUSWORDFIELDS x87StatusWordFields;
    X87CONTROLWORDFIELDS x87ControlWordFields;
    LASTERROR lastError;
} REGDUMP;

typedef struct
{
    DISASM_ARGTYPE type;
    SEGMENTREG segment;
    char mnemonic[64];
    duint constant;
    duint value;
    duint memvalue;
} DISASM_ARG;

typedef struct
{
    char instruction[64];
    DISASM_INSTRTYPE type;
    int argcount;
    int instr_size;
    DISASM_ARG arg[3];
} DISASM_INSTR;

typedef struct
{
    char color[8]; //hex color-code
    char comment[MAX_COMMENT_SIZE];
} STACK_COMMENT;

typedef struct
{
    int ThreadNumber;
    HANDLE Handle;
    DWORD ThreadId;
    duint ThreadStartAddress;
    duint ThreadLocalBase;
    char threadName[MAX_THREAD_NAME_SIZE];
} THREADINFO;

typedef struct
{
    THREADINFO BasicInfo;
    duint ThreadCip;
    DWORD SuspendCount;
    THREADPRIORITY Priority;
    THREADWAITREASON WaitReason;
    DWORD LastError;
} THREADALLINFO;

typedef struct
{
    int count;
    THREADALLINFO* list;
    int CurrentThread;
} THREADLIST;

typedef struct
{
    ULONG_PTR value; //displacement / addrvalue (rip-relative)
    MEMORY_SIZE size; //byte/word/dword/qword
    char mnemonic[MAX_MNEMONIC_SIZE];
} MEMORY_INFO;

typedef struct
{
    ULONG_PTR value;
    VALUE_SIZE size;
} VALUE_INFO;

typedef struct
{
    DWORD type; //value|memory|addr
    VALUE_INFO value; //immediat
    MEMORY_INFO memory;
    ULONG_PTR addr; //addrvalue (jumps + calls)
    bool branch; //jumps/calls
    bool call; //instruction is a call
    int size;
    char instruction[MAX_MNEMONIC_SIZE * 4];
} BASIC_INSTRUCTION_INFO;

typedef struct
{
    SCRIPTBRANCHTYPE type;
    int dest;
    char branchlabel[256];
} SCRIPTBRANCH;

typedef struct
{
    duint addr;
    duint start;
    duint end;
    bool manual;
    int depth;
} FUNCTION_LOOP_INFO;

//Debugger functions
BRIDGE_IMPEXP const char* DbgInit();
BRIDGE_IMPEXP void DbgExit();
BRIDGE_IMPEXP bool DbgMemRead(duint va, unsigned char* dest, duint size);
BRIDGE_IMPEXP bool DbgMemWrite(duint va, const unsigned char* src, duint size);
BRIDGE_IMPEXP duint DbgMemGetPageSize(duint base);
BRIDGE_IMPEXP duint DbgMemFindBaseAddr(duint addr, duint* size);
BRIDGE_IMPEXP bool DbgCmdExec(const char* cmd);
BRIDGE_IMPEXP bool DbgCmdExecDirect(const char* cmd);
BRIDGE_IMPEXP bool DbgMemMap(MEMMAP* memmap);
BRIDGE_IMPEXP bool DbgIsValidExpression(const char* expression);
BRIDGE_IMPEXP bool DbgIsDebugging();
BRIDGE_IMPEXP bool DbgIsJumpGoingToExecute(duint addr);
BRIDGE_IMPEXP bool DbgGetLabelAt(duint addr, SEGMENTREG segment, char* text);
BRIDGE_IMPEXP bool DbgSetLabelAt(duint addr, const char* text);
BRIDGE_IMPEXP bool DbgGetCommentAt(duint addr, char* text);
BRIDGE_IMPEXP bool DbgSetCommentAt(duint addr, const char* text);
BRIDGE_IMPEXP bool DbgGetBookmarkAt(duint addr);
BRIDGE_IMPEXP bool DbgSetBookmarkAt(duint addr, bool isbookmark);
BRIDGE_IMPEXP bool DbgGetModuleAt(duint addr, char* text);
BRIDGE_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr);
BRIDGE_IMPEXP duint DbgValFromString(const char* string);
BRIDGE_IMPEXP bool DbgGetRegDump(REGDUMP* regdump);
BRIDGE_IMPEXP bool DbgValToString(const char* string, duint value);
BRIDGE_IMPEXP bool DbgMemIsValidReadPtr(duint addr);
BRIDGE_IMPEXP int DbgGetBpList(BPXTYPE type, BPMAP* list);
BRIDGE_IMPEXP FUNCTYPE DbgGetFunctionTypeAt(duint addr);
BRIDGE_IMPEXP LOOPTYPE DbgGetLoopTypeAt(duint addr, int depth);
BRIDGE_IMPEXP duint DbgGetBranchDestination(duint addr);
BRIDGE_IMPEXP void DbgScriptLoad(const char* filename);
BRIDGE_IMPEXP void DbgScriptUnload();
BRIDGE_IMPEXP void DbgScriptRun(int destline);
BRIDGE_IMPEXP void DbgScriptStep();
BRIDGE_IMPEXP bool DbgScriptBpToggle(int line);
BRIDGE_IMPEXP bool DbgScriptBpGet(int line);
BRIDGE_IMPEXP bool DbgScriptCmdExec(const char* command);
BRIDGE_IMPEXP void DbgScriptAbort();
BRIDGE_IMPEXP SCRIPTLINETYPE DbgScriptGetLineType(int line);
BRIDGE_IMPEXP void DbgScriptSetIp(int line);
BRIDGE_IMPEXP bool DbgScriptGetBranchInfo(int line, SCRIPTBRANCH* info);
BRIDGE_IMPEXP void DbgSymbolEnum(duint base, CBSYMBOLENUM cbSymbolEnum, void* user);
BRIDGE_IMPEXP bool DbgAssembleAt(duint addr, const char* instruction);
BRIDGE_IMPEXP duint DbgModBaseFromName(const char* name);
BRIDGE_IMPEXP void DbgDisasmAt(duint addr, DISASM_INSTR* instr);
BRIDGE_IMPEXP bool DbgStackCommentGet(duint addr, STACK_COMMENT* comment);
BRIDGE_IMPEXP void DbgGetThreadList(THREADLIST* list);
BRIDGE_IMPEXP void DbgSettingsUpdated();
BRIDGE_IMPEXP void DbgDisasmFastAt(duint addr, BASIC_INSTRUCTION_INFO* basicinfo);
BRIDGE_IMPEXP void DbgMenuEntryClicked(int hEntry);
BRIDGE_IMPEXP bool DbgFunctionGet(duint addr, duint* start, duint* end);
BRIDGE_IMPEXP bool DbgFunctionOverlaps(duint start, duint end);
BRIDGE_IMPEXP bool DbgFunctionAdd(duint start, duint end);
BRIDGE_IMPEXP bool DbgFunctionDel(duint addr);
BRIDGE_IMPEXP bool DbgLoopGet(int depth, duint addr, duint* start, duint* end);
BRIDGE_IMPEXP bool DbgLoopOverlaps(int depth, duint start, duint end);
BRIDGE_IMPEXP bool DbgLoopAdd(duint start, duint end);
BRIDGE_IMPEXP bool DbgLoopDel(int depth, duint addr);
BRIDGE_IMPEXP bool DbgIsRunLocked();
BRIDGE_IMPEXP bool DbgIsBpDisabled(duint addr);
BRIDGE_IMPEXP bool DbgSetAutoCommentAt(duint addr, const char* text);
BRIDGE_IMPEXP void DbgClearAutoCommentRange(duint start, duint end);
BRIDGE_IMPEXP bool DbgSetAutoLabelAt(duint addr, const char* text);
BRIDGE_IMPEXP void DbgClearAutoLabelRange(duint start, duint end);
BRIDGE_IMPEXP bool DbgSetAutoBookmarkAt(duint addr);
BRIDGE_IMPEXP void DbgClearAutoBookmarkRange(duint start, duint end);
BRIDGE_IMPEXP bool DbgSetAutoFunctionAt(duint start, duint end);
BRIDGE_IMPEXP void DbgClearAutoFunctionRange(duint start, duint end);
BRIDGE_IMPEXP bool DbgGetStringAt(duint addr, char* text);
BRIDGE_IMPEXP const DBGFUNCTIONS* DbgFunctions();
BRIDGE_IMPEXP bool DbgWinEvent(MSG* message, long* result);
BRIDGE_IMPEXP bool DbgWinEventGlobal(MSG* message);
BRIDGE_IMPEXP bool DbgIsRunning();
BRIDGE_IMPEXP duint DbgGetTimeWastedCounter();

//Gui defines
#define GUI_PLUGIN_MENU 0
#define GUI_DISASM_MENU 1
#define GUI_DUMP_MENU 2
#define GUI_STACK_MENU 3

#define GUI_DISASSEMBLY 0
#define GUI_DUMP 1
#define GUI_STACK 2

#define GUI_MAX_LINE_SIZE 65536
#define GUI_MAX_DISASSEMBLY_SIZE 2048

//Gui enums
typedef enum
{
    GUI_DISASSEMBLE_AT,             // param1=(duint)va,            param2=(duint)cip
    GUI_SET_DEBUG_STATE,            // param1=(DBGSTATE)state,      param2=unused
    GUI_ADD_MSG_TO_LOG,             // param1=(const char*)msg,     param2=unused
    GUI_CLEAR_LOG,                  // param1=unused,               param2=unused
    GUI_UPDATE_REGISTER_VIEW,       // param1=unused,               param2=unused
    GUI_UPDATE_DISASSEMBLY_VIEW,    // param1=unused,               param2=unused
    GUI_UPDATE_BREAKPOINTS_VIEW,    // param1=unused,               param2=unused
    GUI_UPDATE_WINDOW_TITLE,        // param1=(const char*)file,    param2=unused
    GUI_GET_WINDOW_HANDLE,          // param1=unused,               param2=unused
    GUI_DUMP_AT,                    // param1=(duint)va             param2=unused
    GUI_SCRIPT_ADD,                 // param1=int count,            param2=const char** lines
    GUI_SCRIPT_CLEAR,               // param1=unused,               param2=unused
    GUI_SCRIPT_SETIP,               // param1=int line,             param2=unused
    GUI_SCRIPT_ERROR,               // param1=int line,             param2=const char* message
    GUI_SCRIPT_SETTITLE,            // param1=const char* title,    param2=unused
    GUI_SCRIPT_SETINFOLINE,         // param1=int line,             param2=const char* info
    GUI_SCRIPT_MESSAGE,             // param1=const char* message,  param2=unused
    GUI_SCRIPT_MSGYN,               // param1=const char* message,  param2=unused
    GUI_SYMBOL_LOG_ADD,             // param1(const char*)msg,      param2=unused
    GUI_SYMBOL_LOG_CLEAR,           // param1=unused,               param2=unused
    GUI_SYMBOL_SET_PROGRESS,        // param1=int percent           param2=unused
    GUI_SYMBOL_UPDATE_MODULE_LIST,  // param1=int count,            param2=SYMBOLMODULEINFO* modules
    GUI_REF_ADDCOLUMN,              // param1=int width,            param2=(const char*)title
    GUI_REF_SETROWCOUNT,            // param1=int rows,             param2=unused
    GUI_REF_GETROWCOUNT,            // param1=unused,               param2=unused
    GUI_REF_DELETEALLCOLUMNS,       // param1=unused,               param2=unused
    GUI_REF_SETCELLCONTENT,         // param1=(CELLINFO*)info,      param2=unused
    GUI_REF_GETCELLCONTENT,         // param1=int row,              param2=int col
    GUI_REF_RELOADDATA,             // param1=unused,               param2=unused
    GUI_REF_SETSINGLESELECTION,     // param1=int index,            param2=bool scroll
    GUI_REF_SETPROGRESS,            // param1=int progress,            param2=unused
    GUI_REF_SETSEARCHSTARTCOL,      // param1=int col               param2=unused
    GUI_STACK_DUMP_AT,              // param1=duint addr,           param2=duint csp
    GUI_UPDATE_DUMP_VIEW,           // param1=unused,               param2=unused
    GUI_UPDATE_THREAD_VIEW,         // param1=unused,               param2=unused
    GUI_ADD_RECENT_FILE,            // param1=(const char*)file,    param2=unused
    GUI_SET_LAST_EXCEPTION,         // param1=unsigned int code,    param2=unused
    GUI_GET_DISASSEMBLY,            // param1=duint addr,           param2=char* text
    GUI_MENU_ADD,                   // param1=int hMenu,            param2=const char* title
    GUI_MENU_ADD_ENTRY,             // param1=int hMenu,            param2=const char* title
    GUI_MENU_ADD_SEPARATOR,         // param1=int hMenu,            param2=unused
    GUI_MENU_CLEAR,                 // param1=int hMenu,            param2=unused
    GUI_SELECTION_GET,              // param1=int hWindow,          param2=SELECTIONDATA* selection
    GUI_SELECTION_SET,              // param1=int hWindow,          param2=const SELECTIONDATA* selection
    GUI_GETLINE_WINDOW,             // param1=const char* title,    param2=char* text
    GUI_AUTOCOMPLETE_ADDCMD,        // param1=const char* cmd,      param2=ununsed
    GUI_AUTOCOMPLETE_DELCMD,        // param1=const char* cmd,      param2=ununsed
    GUI_AUTOCOMPLETE_CLEARALL,      // param1=unused,              param2=unused
    GUI_SCRIPT_ENABLEHIGHLIGHTING,  // param1=bool enable,          param2=unused
    GUI_ADD_MSG_TO_STATUSBAR,       // param1=const char* msg,      param2=unused
    GUI_UPDATE_SIDEBAR,             // param1=unused,               param2=unused
    GUI_REPAINT_TABLE_VIEW,         // param1=unused,               param2=unused
    GUI_UPDATE_PATCHES,             // param1=unused,               param2=unused
    GUI_UPDATE_CALLSTACK,           // param1=unused,               param2=unused
    GUI_SYMBOL_REFRESH_CURRENT,     // param1=unused,               param2=unused
    GUI_UPDATE_MEMORY_VIEW,         // param1=unused,               param2=unused
    GUI_REF_INITIALIZE,             // param1=const char* name,     param2=unused
    GUI_LOAD_SOURCE_FILE,           // param1=const char* path,     param2=line
    GUI_MENU_SET_ICON,              // param1=int hMenu,            param2=ICONINFO*
    GUI_MENU_SET_ENTRY_ICON,        // param1=int hEntry,           param2=ICONINFO*
    GUI_SHOW_CPU,                   // param1=unused,               param2=unused
    GUI_ADD_QWIDGET_TAB,            // param1=QWidget*,             param2=unused
    GUI_SHOW_QWIDGET_TAB,           // param1=QWidget*,             param2=unused
    GUI_CLOSE_QWIDGET_TAB,          // param1=QWidget*,             param2=unused
    GUI_EXECUTE_ON_GUI_THREAD,      // param1=GUICALLBACK,          param2=unused
    GUI_UPDATE_TIME_WASTED_COUNTER, // param1=unused,               param2=unused
    GUI_SET_GLOBAL_NOTES,           // param1=const char* text,     param2=unused
    GUI_GET_GLOBAL_NOTES,           // param1=char** text,          param2=unused
    GUI_SET_DEBUGGEE_NOTES,         // param1=const char* text,     param2=unused
    GUI_GET_DEBUGGEE_NOTES          // param1=char** text,          param2=unused
} GUIMSG;

//GUI Typedefs
typedef void (*GUICALLBACK)();

//GUI structures
typedef struct
{
    int row;
    int col;
    const char* str;
} CELLINFO;

typedef struct
{
    duint start;
    duint end;
} SELECTIONDATA;

typedef struct
{
    const void* data;
    duint size;
} ICONDATA;

//GUI functions
BRIDGE_IMPEXP void GuiDisasmAt(duint addr, duint cip);
BRIDGE_IMPEXP void GuiSetDebugState(DBGSTATE state);
BRIDGE_IMPEXP void GuiAddLogMessage(const char* msg);
BRIDGE_IMPEXP void GuiLogClear();
BRIDGE_IMPEXP void GuiUpdateAllViews();
BRIDGE_IMPEXP void GuiUpdateRegisterView();
BRIDGE_IMPEXP void GuiUpdateDisassemblyView();
BRIDGE_IMPEXP void GuiUpdateBreakpointsView();
BRIDGE_IMPEXP void GuiUpdateWindowTitle(const char* filename);
BRIDGE_IMPEXP HWND GuiGetWindowHandle();
BRIDGE_IMPEXP void GuiDumpAt(duint va);
BRIDGE_IMPEXP void GuiScriptAdd(int count, const char** lines);
BRIDGE_IMPEXP void GuiScriptClear();
BRIDGE_IMPEXP void GuiScriptSetIp(int line);
BRIDGE_IMPEXP void GuiScriptError(int line, const char* message);
BRIDGE_IMPEXP void GuiScriptSetTitle(const char* title);
BRIDGE_IMPEXP void GuiScriptSetInfoLine(int line, const char* info);
BRIDGE_IMPEXP void GuiScriptMessage(const char* message);
BRIDGE_IMPEXP int GuiScriptMsgyn(const char* message);
BRIDGE_IMPEXP void GuiScriptEnableHighlighting(bool enable);
BRIDGE_IMPEXP void GuiSymbolLogAdd(const char* message);
BRIDGE_IMPEXP void GuiSymbolLogClear();
BRIDGE_IMPEXP void GuiSymbolSetProgress(int percent);
BRIDGE_IMPEXP void GuiSymbolUpdateModuleList(int count, SYMBOLMODULEINFO* modules);
BRIDGE_IMPEXP void GuiSymbolRefreshCurrent();
BRIDGE_IMPEXP void GuiReferenceAddColumn(int width, const char* title);
BRIDGE_IMPEXP void GuiReferenceSetRowCount(int count);
BRIDGE_IMPEXP int GuiReferenceGetRowCount();
BRIDGE_IMPEXP void GuiReferenceDeleteAllColumns();
BRIDGE_IMPEXP void GuiReferenceInitialize(const char* name);
BRIDGE_IMPEXP void GuiReferenceSetCellContent(int row, int col, const char* str);
BRIDGE_IMPEXP const char* GuiReferenceGetCellContent(int row, int col);
BRIDGE_IMPEXP void GuiReferenceReloadData();
BRIDGE_IMPEXP void GuiReferenceSetSingleSelection(int index, bool scroll);
BRIDGE_IMPEXP void GuiReferenceSetProgress(int progress);
BRIDGE_IMPEXP void GuiReferenceSetSearchStartCol(int col);
BRIDGE_IMPEXP void GuiStackDumpAt(duint addr, duint csp);
BRIDGE_IMPEXP void GuiUpdateDumpView();
BRIDGE_IMPEXP void GuiUpdateThreadView();
BRIDGE_IMPEXP void GuiUpdateMemoryView();
BRIDGE_IMPEXP void GuiAddRecentFile(const char* file);
BRIDGE_IMPEXP void GuiSetLastException(unsigned int exception);
BRIDGE_IMPEXP bool GuiGetDisassembly(duint addr, char* text);
BRIDGE_IMPEXP int GuiMenuAdd(int hMenu, const char* title);
BRIDGE_IMPEXP int GuiMenuAddEntry(int hMenu, const char* title);
BRIDGE_IMPEXP void GuiMenuAddSeparator(int hMenu);
BRIDGE_IMPEXP void GuiMenuClear(int hMenu);
BRIDGE_IMPEXP bool GuiSelectionGet(int hWindow, SELECTIONDATA* selection);
BRIDGE_IMPEXP bool GuiSelectionSet(int hWindow, const SELECTIONDATA* selection);
BRIDGE_IMPEXP bool GuiGetLineWindow(const char* title, char* text);
BRIDGE_IMPEXP void GuiAutoCompleteAddCmd(const char* cmd);
BRIDGE_IMPEXP void GuiAutoCompleteDelCmd(const char* cmd);
BRIDGE_IMPEXP void GuiAutoCompleteClearAll();
BRIDGE_IMPEXP void GuiAddStatusBarMessage(const char* msg);
BRIDGE_IMPEXP void GuiUpdateSideBar();
BRIDGE_IMPEXP void GuiRepaintTableView();
BRIDGE_IMPEXP void GuiUpdatePatches();
BRIDGE_IMPEXP void GuiUpdateCallStack();
BRIDGE_IMPEXP void GuiLoadSourceFile(const char* path, int line);
BRIDGE_IMPEXP void GuiMenuSetIcon(int hMenu, const ICONDATA* icon);
BRIDGE_IMPEXP void GuiMenuSetEntryIcon(int hEntry, const ICONDATA* icon);
BRIDGE_IMPEXP void GuiShowCpu();
BRIDGE_IMPEXP void GuiAddQWidgetTab(void* qWidget);
BRIDGE_IMPEXP void GuiShowQWidgetTab(void* qWidget);
BRIDGE_IMPEXP void GuiCloseQWidgetTab(void* qWidget);
BRIDGE_IMPEXP void GuiExecuteOnGuiThread(GUICALLBACK cbGuiThread);
BRIDGE_IMPEXP void GuiUpdateTimeWastedCounter();
BRIDGE_IMPEXP void GuiSetGlobalNotes(const char* text);
BRIDGE_IMPEXP void GuiGetGlobalNotes(char** text);
BRIDGE_IMPEXP void GuiSetDebuggeeNotes(const char* text);
BRIDGE_IMPEXP void GuiGetDebuggeeNotes(char** text);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif // _BRIDGEMAIN_H_

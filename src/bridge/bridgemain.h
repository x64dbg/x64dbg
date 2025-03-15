#ifndef _BRIDGEMAIN_H_
#define _BRIDGEMAIN_H_

#include <Windows.h>
#include <stdint.h>

#ifndef __cplusplus
#include <stdbool.h>
#define DEFAULT_PARAM(name, value) name
#else
#define DEFAULT_PARAM(name, value) name = value
#endif // __cplusplus

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
#endif // __cplusplus

//Bridge defines
#define MAX_SETTING_SIZE 65536
#define DBG_VERSION 25

//Bridge functions

/// <summary>
/// Initialize the bridge.
/// </summary>
/// <returns>On error it returns a non-null error message.</returns>
BRIDGE_IMPEXP const wchar_t* BridgeInit();

BRIDGE_IMPEXP HMODULE WINAPI BridgeLoadLibraryCheckedW(const wchar_t* szDll, bool allowFailure);

BRIDGE_IMPEXP HMODULE WINAPI BridgeLoadLibraryCheckedA(const char* szDll, bool allowFailure);

/// <summary>
/// Start the bridge.
/// </summary>
/// <returns>On error it returns a non-null error message.</returns>
BRIDGE_IMPEXP const wchar_t* BridgeStart();

/// <summary>
/// Allocate buffer. Use BridgeFree to free the buffer.
/// </summary>
/// <param name="size">Size in bytes of the buffer to allocate.</param>
/// <returns>A pointer to the allocated buffer. This function will trigger a crash dump if unsuccessful.</returns>
BRIDGE_IMPEXP void* BridgeAlloc(size_t size);

/// <summary>
/// Free buffer allocated by BridgeAlloc.
/// </summary>
/// <param name="ptr">Buffer to free.</param>
BRIDGE_IMPEXP void BridgeFree(void* ptr);

/// <summary>
/// Get a string setting from the in-memory setting store.
/// </summary>
/// <param name="section">Section the setting is in. Cannot be null.</param>
/// <param name="key">Setting key (name). Cannot be null.</param>
/// <param name="value">Output buffer for the value. Should be of MAX_SETTING_SIZE. Cannot be null.</param>
/// <returns>True if the setting was found and copied in the value parameter.</returns>
BRIDGE_IMPEXP bool BridgeSettingGet(const char* section, const char* key, char* value);

/// <summary>
/// Get an integer setting from the in-memory setting store.
/// </summary>
/// <param name="section">Section the setting is in. Cannot be null.</param>
/// <param name="key">Setting key (name). Cannot be null.</param>
/// <param name="value">Output value.</param>
/// <returns>True if the setting was found and successfully converted to an integer.</returns>
BRIDGE_IMPEXP bool BridgeSettingGetUint(const char* section, const char* key, duint* value);

/// <summary>
/// Set a string setting in the in-memory setting store.
/// </summary>
/// <param name="section">Section the setting is in. Cannot be null.</param>
/// <param name="key">Setting key (name). Set to null to clear the whole section.</param>
/// <param name="value">New setting value. Set to null to remove the key from the section.</param>
/// <returns>True if the operation was successful.</returns>
BRIDGE_IMPEXP bool BridgeSettingSet(const char* section, const char* key, const char* value);

/// <summary>
/// Set an integer setting in the in-memory setting store.
/// </summary>
/// <param name="section">Section the setting is in. Cannot be null.</param>
/// <param name="key">Setting key (name). Set to null to clear the whole section.</param>
/// <param name="value">New setting value.</param>
/// <returns>True if the operation was successful.</returns>
BRIDGE_IMPEXP bool BridgeSettingSetUint(const char* section, const char* key, duint value);

/// <summary>
/// Flush the in-memory setting store to disk.
/// </summary>
/// <returns></returns>
BRIDGE_IMPEXP bool BridgeSettingFlush();

/// <summary>
/// Read the in-memory setting store from disk.
/// </summary>
/// <param name="errorLine">Line where the error occurred. Set to null to ignore this.</param>
/// <returns>True if the setting were read and parsed correctly.</returns>
BRIDGE_IMPEXP bool BridgeSettingRead(int* errorLine);

/// <summary>
/// Get the debugger version.
/// </summary>
/// <returns>25</returns>
BRIDGE_IMPEXP int BridgeGetDbgVersion();

/// <summary>
/// Checks if the current process is elevated.
/// </summary>
/// <returns>true if the process is elevated, false otherwise.</returns>
BRIDGE_IMPEXP bool BridgeIsProcessElevated();

/// <summary>
/// Gets the NT build number from the operating system.
/// </summary>
/// <returns>NtBuildNumber</returns>
BRIDGE_IMPEXP unsigned int BridgeGetNtBuildNumber();

/// <summary>
/// Returns the user directory (without trailing backslash).
/// </summary>
BRIDGE_IMPEXP const wchar_t* BridgeUserDirectory();

/// <summary>
/// Returns true if x64dbg is running under ARM64 emulation.
/// </summary>
BRIDGE_IMPEXP bool BridgeIsARM64Emulated();

#ifdef __cplusplus
}
#endif // __cplusplus

//list structure (and C++ wrapper)
#include "bridgelist.h"

#include "bridgegraph.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

//Debugger defines
#define MAX_LABEL_SIZE 256
#define MAX_COMMENT_SIZE 512
#define MAX_MODULE_SIZE 256
#define MAX_IMPORT_SIZE 65536
#define MAX_BREAKPOINT_SIZE 256
#define MAX_CONDITIONAL_EXPR_SIZE 256
#define MAX_CONDITIONAL_TEXT_SIZE 256
#define MAX_SCRIPT_LINE_SIZE 2048
#define MAX_THREAD_NAME_SIZE 256
#define MAX_WATCH_NAME_SIZE 256
#define MAX_STRING_SIZE 512
#define MAX_ERROR_SIZE 512
#define RIGHTS_STRING_SIZE (sizeof("ERWCG"))
#define MAX_SECTION_SIZE 10
#define MAX_COMMAND_LINE_SIZE 256
#define MAX_MNEMONIC_SIZE 64

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif // PAGE_SIZE

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
    flagmodule = 0x1,
    flaglabel = 0x2,
    flagcomment = 0x4,
    flagbookmark = 0x8,
    flagfunction = 0x10,
    flagloop = 0x20,
    flagargs = 0x40,
    flagNoFuncOffset = 0x80
} ADDRINFOFLAGS;

typedef enum
{
    bp_none = 0,
    bp_normal = 1,
    bp_hardware = 2,
    bp_memory = 4,
    bp_dll = 8,
    bp_exception = 16
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
    LOOP_END,
    LOOP_SINGLE
} LOOPTYPE;

//order by most important type last
typedef enum
{
    XREF_NONE,
    XREF_DATA,
    XREF_JMP,
    XREF_CALL
} XREFTYPE;

typedef enum
{
    ARG_NONE,
    ARG_BEGIN,
    ARG_MIDDLE,
    ARG_END,
    ARG_SINGLE
} ARGTYPE;

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
    DBG_GET_TIME_WASTED_COUNTER,    // param1=unused,                    param2=unused
    DBG_SYMBOL_ENUM_FROMCACHE,      // param1=SYMBOLCBINFO* cbInfo,      param2=unused
    DBG_DELETE_COMMENT_RANGE,       // param1=duint start,               param2=duint end
    DBG_DELETE_LABEL_RANGE,         // param1=duint start,               param2=duint end
    DBG_DELETE_BOOKMARK_RANGE,      // param1=duint start,               param2=duint end
    DBG_GET_XREF_COUNT_AT,          // param1=duint addr,                param2=unused
    DBG_GET_XREF_TYPE_AT,           // param1=duint addr,                param2=unused
    DBG_XREF_ADD,                   // param1=duint addr,                param2=duint from
    DBG_XREF_DEL_ALL,               // param1=duint addr,                param2=unused
    DBG_XREF_GET,                   // param1=duint addr,                param2=XREF_INFO* info
    DBG_GET_ENCODE_TYPE_BUFFER,     // param1=duint addr,                param2=unused
    DBG_ENCODE_TYPE_GET,            // param1=duint addr,                param2=duint size
    DBG_DELETE_ENCODE_TYPE_RANGE,   // param1=duint start,               param2=duint end
    DBG_ENCODE_SIZE_GET,            // param1=duint addr,                param2=duint codesize
    DBG_DELETE_ENCODE_TYPE_SEG,     // param1=duint addr,                param2=unused
    DBG_RELEASE_ENCODE_TYPE_BUFFER, // param1=void* buffer,              param2=unused
    DBG_ARGUMENT_GET,               // param1=FUNCTION* info,            param2=unused
    DBG_ARGUMENT_OVERLAPS,          // param1=FUNCTION* info,            param2=unused
    DBG_ARGUMENT_ADD,               // param1=FUNCTION* info,            param2=unused
    DBG_ARGUMENT_DEL,               // param1=FUNCTION* info,            param2=unused
    DBG_GET_WATCH_LIST,             // param1=ListOf(WATCHINFO),         param2=unused
    DBG_SELCHANGED,                 // param1=hWindow,                   param2=VA
    DBG_GET_PROCESS_HANDLE,         // param1=unused,                    param2=unused
    DBG_GET_THREAD_HANDLE,          // param1=unused,                    param2=unused
    DBG_GET_PROCESS_ID,             // param1=unused,                    param2=unused
    DBG_GET_THREAD_ID,              // param1=unused,                    param2=unused
    DBG_GET_PEB_ADDRESS,            // param1=DWORD ProcessId,           param2=unused
    DBG_GET_TEB_ADDRESS,            // param1=DWORD ThreadId,            param2=unused
    DBG_ANALYZE_FUNCTION,           // param1=BridgeCFGraphList* graph,  param2=duint entry
    DBG_MENU_PREPARE,               // param1=int hMenu,                 param2=unused
    DBG_GET_SYMBOL_INFO,            // param1=void* symbol,              param2=SYMBOLINFO* info
    DBG_GET_DEBUG_ENGINE,           // param1=unused,                    param2-unused
    DBG_GET_SYMBOL_INFO_AT,         // param1=duint addr,                param2=SYMBOLINFO* info
    DBG_XREF_ADD_MULTI,             // param1=const XREF_EDGE* edges,    param2=duint count
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
    size_qword = 8,
    size_xmmword = 16,
    size_ymmword = 32
} MEMORY_SIZE;

typedef enum
{
    enc_unknown,  //must be 0
    enc_byte,     //1 byte
    enc_word,     //2 bytes
    enc_dword,    //4 bytes
    enc_fword,    //6 bytes
    enc_qword,    //8 bytes
    enc_tbyte,    //10 bytes
    enc_oword,    //16 bytes
    enc_mmword,   //8 bytes
    enc_xmmword,  //16 bytes
    enc_ymmword,  //32 bytes
    enc_zmmword,  //64 bytes avx512 not supported
    enc_real4,    //4 byte float
    enc_real8,    //8 byte double
    enc_real10,   //10 byte decimal
    enc_ascii,    //ascii sequence
    enc_unicode,  //unicode sequence
    enc_code,     //start of code
    enc_junk,     //junk code
    enc_middle    //middle of data
} ENCODETYPE;

typedef enum
{
    TYPE_UINT, // unsigned integer
    TYPE_INT,  // signed integer
    TYPE_FLOAT,// single precision floating point value
    TYPE_ASCII, // ascii string
    TYPE_UNICODE, // unicode string
    TYPE_INVALID // invalid watch expression or data type
} WATCHVARTYPE;

typedef enum
{
    MODE_DISABLED, // watchdog is disabled
    MODE_ISTRUE,   // alert if expression is not 0
    MODE_ISFALSE,  // alert if expression is 0
    MODE_CHANGED,  // alert if expression is changed
    MODE_UNCHANGED // alert if expression is not changed
} WATCHDOGMODE;

typedef enum
{
    hw_access,
    hw_write,
    hw_execute
} BPHWTYPE;

typedef enum
{
    mem_access,
    mem_read,
    mem_write,
    mem_execute
} BPMEMTYPE;

typedef enum
{
    dll_load = 1,
    dll_unload,
    dll_all
} BPDLLTYPE;

typedef enum
{
    ex_firstchance = 1,
    ex_secondchance,
    ex_all
} BPEXTYPE;

typedef enum
{
    hw_byte,
    hw_word,
    hw_dword,
    hw_qword
} BPHWSIZE;

typedef enum
{
    sym_import,
    sym_export,
    sym_symbol
} SYMBOLTYPE;

#define SYMBOL_MASK_IMPORT (1u << sym_import)
#define SYMBOL_MASK_EXPORT (1u << sym_export)
#define SYMBOL_MASK_SYMBOL (1u << sym_symbol)
#define SYMBOL_MASK_ALL (SYMBOL_MASK_IMPORT | SYMBOL_MASK_EXPORT | SYMBOL_MASK_SYMBOL)

typedef enum
{
    mod_user,
    mod_system
} MODULEPARTY;

typedef enum
{
    DebugEngineTitanEngine,
    DebugEngineGleeBug,
    DebugEngineStaticEngine,
} DEBUG_ENGINE;

//Debugger typedefs
typedef MEMORY_SIZE VALUE_SIZE;

typedef struct DBGFUNCTIONS_ DBGFUNCTIONS;

// Callback declaration:
// bool cbSymbolEnum(const SYMBOLPTR* symbol, void* user);
// To get the data from the opaque pointer:
// SYMBOLINFO info;
// DbgGetSymbolInfo(symbol, &info);
// The SYMBOLPTR* becomes invalid when the module is unloaded
// DO NOT STORE unless you are absolutely certain you handle it correctly
typedef bool (*CBSYMBOLENUM)(const struct SYMBOLPTR_* symbol, void* user);

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
    // extended part
    unsigned char typeEx; //BPHWTYPE/BPMEMTYPE/BPDLLTYPE/BPEXTYPE
    unsigned char hwSize; //BPHWSIZE
    unsigned int hitCount;
    bool fastResume;
    bool silent;
    char breakCondition[MAX_CONDITIONAL_EXPR_SIZE];
    char logText[MAX_CONDITIONAL_TEXT_SIZE];
    char logCondition[MAX_CONDITIONAL_EXPR_SIZE];
    char commandText[MAX_CONDITIONAL_TEXT_SIZE];
    char commandCondition[MAX_CONDITIONAL_EXPR_SIZE];
} BRIDGEBP;

typedef struct
{
    int count;
    BRIDGEBP* bp;
} BPMAP;

typedef struct
{
    char WatchName[MAX_WATCH_NAME_SIZE];
    char Expression[MAX_CONDITIONAL_EXPR_SIZE];
    unsigned int window;
    unsigned int id;
    WATCHVARTYPE varType;
    WATCHDOGMODE watchdogMode;
    duint value;
    bool watchdogTriggered;
} WATCHINFO;

typedef struct
{
    duint start; //OUT
    duint end; //OUT
    duint instrcount; //OUT
} FUNCTION;

typedef struct
{
    int depth; //IN
    duint start; //OUT
    duint end; //OUT
    duint instrcount; //OUT
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
    FUNCTION args;
} BRIDGE_ADDRINFO;

typedef struct SYMBOLINFO_
{
    duint addr;
    char* decoratedSymbol;
    char* undecoratedSymbol;
    SYMBOLTYPE type;

    // If true: Use BridgeFree(decoratedSymbol) to deallocate
    // Else: The decoratedSymbol pointer is valid until the module unloads
    bool freeDecorated;

    // If true: Use BridgeFree(undecoratedSymbol) to deallcoate
    // Else: The undecoratedSymbol pointer is valid until the module unloads
    bool freeUndecorated;

    // The entry point pseudo-export has ordinal == 0 (invalid ordinal value)
    DWORD ordinal;
} SYMBOLINFO;

#ifdef __cplusplus
struct SYMBOLINFOCPP : SYMBOLINFO
{
    SYMBOLINFOCPP(const SYMBOLINFOCPP &) = delete;
    SYMBOLINFOCPP(SYMBOLINFOCPP &&) = delete;

    SYMBOLINFOCPP()
    {
        memset(this, 0, sizeof(SYMBOLINFO));
    }

    ~SYMBOLINFOCPP()
    {
        if(freeDecorated)
            BridgeFree(decoratedSymbol);
        if(freeUndecorated)
            BridgeFree(undecoratedSymbol);
    }
};
static_assert(sizeof(SYMBOLINFOCPP) == sizeof(SYMBOLINFO), "");
#endif // __cplusplus

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
    duint start;
    duint end;
    unsigned int symbolMask;
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
    bool ES;
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
    char name[128];
} LASTERROR;

typedef struct
{
    DWORD code;
    char name[128];
} LASTSTATUS;

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
    LASTSTATUS lastStatus;
} REGDUMP;

typedef struct
{
    DISASM_ARGTYPE type; //normal/memory
    SEGMENTREG segment;
    char mnemonic[64];
    duint constant; //constant in the instruction (imm/disp)
    duint value; //equal to constant or equal to the register value
    duint memvalue; //memsize:[value]
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
    FILETIME UserTime;
    FILETIME KernelTime;
    FILETIME CreationTime;
    ULONG64 Cycles; // Windows Vista or greater
} THREADALLINFO;

typedef struct
{
    int count;
    THREADALLINFO* list;
    int CurrentThread;
} THREADLIST;

typedef struct
{
    duint value; //displacement / addrvalue (rip-relative)
    MEMORY_SIZE size; //byte/word/dword/qword
    char mnemonic[MAX_MNEMONIC_SIZE];
} MEMORY_INFO;

typedef struct
{
    duint value;
    VALUE_SIZE size;
} VALUE_INFO;

//definitions for BASIC_INSTRUCTION_INFO.type
#define TYPE_VALUE 1
#define TYPE_MEMORY 2
#define TYPE_ADDR 4

typedef struct
{
    DWORD type; //value|memory|addr
    VALUE_INFO value; //immediat
    MEMORY_INFO memory;
    duint addr; //addrvalue (jumps + calls)
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

typedef struct
{
    duint addr;
    XREFTYPE type;
} XREF_RECORD;

typedef struct
{
    duint refcount;
    XREF_RECORD* references;
} XREF_INFO;

typedef struct
{
    duint address;
    duint from;
} XREF_EDGE;

typedef struct SYMBOLPTR_
{
    duint modbase;
    const void* symbol;
} SYMBOLPTR;

//Debugger functions
BRIDGE_IMPEXP const char* DbgInit();
BRIDGE_IMPEXP void DbgExit();
BRIDGE_IMPEXP bool DbgMemRead(duint va, void* dest, duint size);
BRIDGE_IMPEXP bool DbgMemWrite(duint va, const void* src, duint size);
BRIDGE_IMPEXP duint DbgMemGetPageSize(duint base);
BRIDGE_IMPEXP duint DbgMemFindBaseAddr(duint addr, duint* size);

/// <summary>
/// Asynchronously execute a debugger command by adding it to the command queue.
/// Note: the command may not have completed before this call returns. Use this
/// function if you don't care when the command gets executed.
///
/// Example: DbgCmdExec("ClearLog")
/// </summary>
/// <param name="cmd">The command to execute.</param>
/// <returns>True if the command was successfully submitted to the command queue. False if the submission failed.</returns>
BRIDGE_IMPEXP bool DbgCmdExec(const char* cmd);

/// <summary>
/// Performs synchronous execution of a debugger command. This function call only
/// returns after the command has completed.
///
/// Example: DbgCmdExecDirect("loadlib advapi32.dll")
/// </summary>
/// <param name="cmd">The command to execute.</param>
/// <returns>True if the command executed successfully, False if there was a problem.</returns>
BRIDGE_IMPEXP bool DbgCmdExecDirect(const char* cmd);
BRIDGE_IMPEXP bool DbgMemMap(MEMMAP* memmap);
BRIDGE_IMPEXP bool DbgIsValidExpression(const char* expression);
BRIDGE_IMPEXP bool DbgIsDebugging();
BRIDGE_IMPEXP bool DbgIsJumpGoingToExecute(duint addr);
BRIDGE_IMPEXP bool DbgGetLabelAt(duint addr, SEGMENTREG segment, char* text);
BRIDGE_IMPEXP bool DbgSetLabelAt(duint addr, const char* text);
BRIDGE_IMPEXP void DbgClearLabelRange(duint start, duint end);
BRIDGE_IMPEXP bool DbgGetCommentAt(duint addr, char* text);
BRIDGE_IMPEXP bool DbgSetCommentAt(duint addr, const char* text);
BRIDGE_IMPEXP void DbgClearCommentRange(duint start, duint end);
BRIDGE_IMPEXP bool DbgGetBookmarkAt(duint addr);
BRIDGE_IMPEXP bool DbgSetBookmarkAt(duint addr, bool isbookmark);
BRIDGE_IMPEXP void DbgClearBookmarkRange(duint start, duint end);
BRIDGE_IMPEXP bool DbgGetModuleAt(duint addr, char* text);
BRIDGE_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr);
BRIDGE_IMPEXP duint DbgValFromString(const char* string);
BRIDGE_IMPEXP bool DbgGetRegDumpEx(REGDUMP* regdump, size_t size);
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
BRIDGE_IMPEXP bool DbgSymbolEnum(duint base, CBSYMBOLENUM cbSymbolEnum, void* user);
BRIDGE_IMPEXP bool DbgSymbolEnumFromCache(duint base, CBSYMBOLENUM cbSymbolEnum, void* user);
BRIDGE_IMPEXP bool DbgSymbolEnumRange(duint start, duint end, unsigned int symbolMask, CBSYMBOLENUM cbSymbolEnum, void* user);
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
BRIDGE_IMPEXP bool DbgArgumentGet(duint addr, duint* start, duint* end);
BRIDGE_IMPEXP bool DbgArgumentOverlaps(duint start, duint end);
BRIDGE_IMPEXP bool DbgArgumentAdd(duint start, duint end);
BRIDGE_IMPEXP bool DbgArgumentDel(duint addr);
BRIDGE_IMPEXP bool DbgLoopGet(int depth, duint addr, duint* start, duint* end);
BRIDGE_IMPEXP bool DbgLoopOverlaps(int depth, duint start, duint end);
BRIDGE_IMPEXP bool DbgLoopAdd(duint start, duint end);
BRIDGE_IMPEXP bool DbgLoopDel(int depth, duint addr);
BRIDGE_IMPEXP bool DbgXrefAdd(duint addr, duint from);
BRIDGE_IMPEXP bool DbgXrefDelAll(duint addr);
BRIDGE_IMPEXP bool DbgXrefGet(duint addr, XREF_INFO* info);
BRIDGE_IMPEXP size_t DbgGetXrefCountAt(duint addr);
BRIDGE_IMPEXP XREFTYPE DbgGetXrefTypeAt(duint addr);
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
BRIDGE_IMPEXP ARGTYPE DbgGetArgTypeAt(duint addr);
BRIDGE_IMPEXP void* DbgGetEncodeTypeBuffer(duint addr, duint* size);
BRIDGE_IMPEXP void DbgReleaseEncodeTypeBuffer(void* buffer);
BRIDGE_IMPEXP ENCODETYPE DbgGetEncodeTypeAt(duint addr, duint size);
BRIDGE_IMPEXP duint DbgGetEncodeSizeAt(duint addr, duint codesize);
BRIDGE_IMPEXP bool DbgSetEncodeType(duint addr, duint size, ENCODETYPE type);
BRIDGE_IMPEXP void DbgDelEncodeTypeRange(duint start, duint end);
BRIDGE_IMPEXP void DbgDelEncodeTypeSegment(duint start);
BRIDGE_IMPEXP bool DbgGetWatchList(ListOf(WATCHINFO) list);
BRIDGE_IMPEXP void DbgSelChanged(int hWindow, duint VA);
BRIDGE_IMPEXP HANDLE DbgGetProcessHandle();
BRIDGE_IMPEXP HANDLE DbgGetThreadHandle();
BRIDGE_IMPEXP DWORD DbgGetProcessId();
BRIDGE_IMPEXP DWORD DbgGetThreadId();
BRIDGE_IMPEXP duint DbgGetPebAddress(DWORD ProcessId);
BRIDGE_IMPEXP duint DbgGetTebAddress(DWORD ThreadId);
BRIDGE_IMPEXP bool DbgAnalyzeFunction(duint entry, BridgeCFGraphList* graph);
BRIDGE_IMPEXP duint DbgEval(const char* expression, bool* DEFAULT_PARAM(success, nullptr));
BRIDGE_IMPEXP void DbgGetSymbolInfo(const SYMBOLPTR* symbolptr, SYMBOLINFO* info);
BRIDGE_IMPEXP DEBUG_ENGINE DbgGetDebugEngine();
BRIDGE_IMPEXP bool DbgGetSymbolInfoAt(duint addr, SYMBOLINFO* info);
BRIDGE_IMPEXP duint DbgXrefAddMulti(const XREF_EDGE* edges, duint count);

//Gui defines
typedef enum
{
    GUI_PLUGIN_MENU,
    GUI_DISASM_MENU,
    GUI_DUMP_MENU,
    GUI_STACK_MENU,
    GUI_GRAPH_MENU,
    GUI_MEMMAP_MENU,
    GUI_SYMMOD_MENU,
} GUIMENUTYPE;

BRIDGE_IMPEXP void DbgMenuPrepare(GUIMENUTYPE hMenu);

typedef enum
{
    GUI_DISASSEMBLY,
    GUI_DUMP,
    GUI_STACK,
    GUI_GRAPH,
    GUI_MEMMAP,
    GUI_SYMMOD,
    GUI_THREADS,
} GUISELECTIONTYPE;

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
    GUI_REF_SETPROGRESS,            // param1=int progress,         param2=unused
    GUI_REF_SETCURRENTTASKPROGRESS, // param1=int progress,         param2=const char* taskTitle
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
    GUI_SELECTION_GET,              // param1=GUISELECTIONTYPE,     param2=SELECTIONDATA* selection
    GUI_SELECTION_SET,              // param1=GUISELECTIONTYPE,     param2=const SELECTIONDATA* selection
    GUI_GETLINE_WINDOW,             // param1=const char* title,    param2=char* text
    GUI_AUTOCOMPLETE_ADDCMD,        // param1=const char* cmd,      param2=ununsed
    GUI_AUTOCOMPLETE_DELCMD,        // param1=const char* cmd,      param2=ununsed
    GUI_AUTOCOMPLETE_CLEARALL,      // param1=unused,               param2=unused
    GUI_SCRIPT_ENABLEHIGHLIGHTING,  // param1=bool enable,          param2=unused
    GUI_ADD_MSG_TO_STATUSBAR,       // param1=const char* msg,      param2=unused
    GUI_UPDATE_SIDEBAR,             // param1=unused,               param2=unused
    GUI_REPAINT_TABLE_VIEW,         // param1=unused,               param2=unused
    GUI_UPDATE_PATCHES,             // param1=unused,               param2=unused
    GUI_UPDATE_CALLSTACK,           // param1=unused,               param2=unused
    GUI_UPDATE_SEHCHAIN,            // param1=unused,               param2=unused
    GUI_SYMBOL_REFRESH_CURRENT,     // param1=unused,               param2=unused
    GUI_UPDATE_MEMORY_VIEW,         // param1=unused,               param2=unused
    GUI_REF_INITIALIZE,             // param1=const char* name,     param2=unused
    GUI_LOAD_SOURCE_FILE,           // param1=const char* path,     param2=duint addr
    GUI_MENU_SET_ICON,              // param1=int hMenu,            param2=ICONINFO*
    GUI_MENU_SET_ENTRY_ICON,        // param1=int hEntry,           param2=ICONINFO*
    GUI_SHOW_CPU,                   // param1=unused,               param2=unused
    GUI_ADD_QWIDGET_TAB,            // param1=QWidget*,             param2=unused
    GUI_SHOW_QWIDGET_TAB,           // param1=QWidget*,             param2=unused
    GUI_CLOSE_QWIDGET_TAB,          // param1=QWidget*,             param2=unused
    GUI_EXECUTE_ON_GUI_THREAD,      // param1=GUICALLBACKEX cb,     param2=void* userdata
    GUI_UPDATE_TIME_WASTED_COUNTER, // param1=unused,               param2=unused
    GUI_SET_GLOBAL_NOTES,           // param1=const char* text,     param2=unused
    GUI_GET_GLOBAL_NOTES,           // param1=char** text,          param2=unused
    GUI_SET_DEBUGGEE_NOTES,         // param1=const char* text,     param2=unused
    GUI_GET_DEBUGGEE_NOTES,         // param1=char** text,          param2=unused
    GUI_DUMP_AT_N,                  // param1=int index,            param2=duint va
    GUI_DISPLAY_WARNING,            // param1=const char *text,     param2=unused
    GUI_REGISTER_SCRIPT_LANG,       // param1=SCRIPTTYPEINFO* info, param2=unused
    GUI_UNREGISTER_SCRIPT_LANG,     // param1=int id,               param2=unused
    GUI_UPDATE_ARGUMENT_VIEW,       // param1=unused,               param2=unused
    GUI_FOCUS_VIEW,                 // param1=int hWindow,          param2=unused
    GUI_UPDATE_WATCH_VIEW,          // param1=unused,               param2=unused
    GUI_LOAD_GRAPH,                 // param1=BridgeCFGraphList*    param2=unused
    GUI_GRAPH_AT,                   // param1=duint addr            param2=unused
    GUI_UPDATE_GRAPH_VIEW,          // param1=unused,               param2=unused
    GUI_SET_LOG_ENABLED,            // param1=bool isEnabled        param2=unused
    GUI_ADD_FAVOURITE_TOOL,         // param1=const char* name      param2=const char* description
    GUI_ADD_FAVOURITE_COMMAND,      // param1=const char* command   param2=const char* shortcut
    GUI_SET_FAVOURITE_TOOL_SHORTCUT,// param1=const char* name      param2=const char* shortcut
    GUI_FOLD_DISASSEMBLY,           // param1=duint startAddress    param2=duint length
    GUI_SELECT_IN_MEMORY_MAP,       // param1=duint addr,           param2=unused
    GUI_GET_ACTIVE_VIEW,            // param1=ACTIVEVIEW*,          param2=unused
    GUI_MENU_SET_ENTRY_CHECKED,     // param1=int hEntry,           param2=bool checked
    GUI_ADD_INFO_LINE,              // param1=const char* infoline, param2=unused
    GUI_PROCESS_EVENTS,             // param1=unused,               param2=unused
    GUI_TYPE_ADDNODE,               // param1=void* parent,         param2=TYPEDESCRIPTOR* type
    GUI_TYPE_CLEAR,                 // param1=unused,               param2=unused
    GUI_UPDATE_TYPE_WIDGET,         // param1=unused,               param2=unused
    GUI_CLOSE_APPLICATION,          // param1=unused,               param2=unused
    GUI_MENU_SET_VISIBLE,           // param1=int hMenu,            param2=bool visible
    GUI_MENU_SET_ENTRY_VISIBLE,     // param1=int hEntry,           param2=bool visible
    GUI_MENU_SET_NAME,              // param1=int hMenu,            param2=const char* name
    GUI_MENU_SET_ENTRY_NAME,        // param1=int hEntry,           param2=const char* name
    GUI_FLUSH_LOG,                  // param1=unused,               param2=unused
    GUI_MENU_SET_ENTRY_HOTKEY,      // param1=int hEntry,           param2=const char* hack
    GUI_REF_SEARCH_GETROWCOUNT,     // param1=unused,               param2=unused
    GUI_REF_SEARCH_GETCELLCONTENT,  // param1=int row,              param2=int col
    GUI_MENU_REMOVE,                // param1=int hEntryMenu,       param2=unused
    GUI_REF_ADDCOMMAND,             // param1=const char* title,    param2=const char* command
    GUI_OPEN_TRACE_FILE,            // param1=const char* file name,param2=unused
    GUI_UPDATE_TRACE_BROWSER,       // param1=unused,               param2=unused
    GUI_INVALIDATE_SYMBOL_SOURCE,   // param1=duint base,           param2=unused
    GUI_GET_CURRENT_GRAPH,          // param1=BridgeCFGraphList*,   param2=unused
    GUI_SHOW_REF,                   // param1=unused,               param2=unused
    GUI_SELECT_IN_SYMBOLS_TAB,      // param1=duint addr,           param2=unused
    GUI_GOTO_TRACE,                 // param1=duint index,          param2=unused
    GUI_SHOW_TRACE,                 // param1=unused,               param2=unused
    GUI_GET_MAIN_THREAD_ID,         // param1=unused,               param2=unused
    GUI_ADD_MSG_TO_LOG_HTML,        // param1=(const char*)msg,     param2=unused
    GUI_IS_LOG_ENABLED,             // param1=unused,               param2=unused
    GUI_IS_DEBUGGER_FOCUSED_UNUSED,        // This message is removed, could be used for future purposes
    GUI_SAVE_LOG,                   // param1=const char* file name,param2=unused
    GUI_REDIRECT_LOG,               // param1=const char* file name,param2=unused
    GUI_STOP_REDIRECT_LOG,          // param1=unused,               param2=unused
    GUI_SHOW_THREADS,               // param1=unused,               param2=unused
} GUIMSG;

//GUI Typedefs
struct _TYPEDESCRIPTOR;

typedef void (*GUICALLBACK)();
typedef void (*GUICALLBACKEX)(void*);
typedef bool (*GUISCRIPTEXECUTE)(const char* text);
typedef void (*GUISCRIPTCOMPLETER)(const char* text, char** entries, int* entryCount);
typedef bool (*TYPETOSTRING)(const struct _TYPEDESCRIPTOR* type, char* dest, size_t* destCount); //don't change destCount for final failure

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

typedef struct
{
    char name[64];
    int id;
    GUISCRIPTEXECUTE execute;
    GUISCRIPTCOMPLETER completeCommand;
} SCRIPTTYPEINFO;

typedef struct
{
    void* titleHwnd;
    void* classHwnd;
    char title[MAX_STRING_SIZE];
    char className[MAX_STRING_SIZE];
} ACTIVEVIEW;

typedef struct _TYPEDESCRIPTOR
{
    bool expanded; //is the type node expanded?
    bool reverse; //big endian?
    const char* name; //type name (int b)
    duint addr; //virtual address
    duint offset; //offset to addr for the actual location
    int id; //type id
    int size; //sizeof(type)
    TYPETOSTRING callback; //convert to string
    void* userdata; //user data
} TYPEDESCRIPTOR;

//GUI functions
//code page is utf8
BRIDGE_IMPEXP const char* GuiTranslateText(const char* Source);
BRIDGE_IMPEXP void GuiDisasmAt(duint addr, duint cip);
BRIDGE_IMPEXP void GuiSetDebugState(DBGSTATE state);
BRIDGE_IMPEXP void GuiSetDebugStateFast(DBGSTATE state);
BRIDGE_IMPEXP void GuiAddLogMessage(const char* msg);
BRIDGE_IMPEXP void GuiAddLogMessageHtml(const char* msg);
BRIDGE_IMPEXP void GuiLogClear();
BRIDGE_IMPEXP void GuiLogSave(const char* filename);
BRIDGE_IMPEXP void GuiLogRedirect(const char* filename);
BRIDGE_IMPEXP void GuiLogRedirectStop();
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
BRIDGE_IMPEXP int GuiReferenceSearchGetRowCount();
BRIDGE_IMPEXP void GuiReferenceDeleteAllColumns();
BRIDGE_IMPEXP void GuiReferenceInitialize(const char* name);
BRIDGE_IMPEXP void GuiReferenceSetCellContent(int row, int col, const char* str);
BRIDGE_IMPEXP char* GuiReferenceGetCellContent(int row, int col);
BRIDGE_IMPEXP char* GuiReferenceSearchGetCellContent(int row, int col);
BRIDGE_IMPEXP void GuiReferenceReloadData();
BRIDGE_IMPEXP void GuiReferenceSetSingleSelection(int index, bool scroll);
BRIDGE_IMPEXP void GuiReferenceSetProgress(int progress);
BRIDGE_IMPEXP void GuiReferenceSetCurrentTaskProgress(int progress, const char* taskTitle);
BRIDGE_IMPEXP void GuiReferenceSetSearchStartCol(int col);
BRIDGE_IMPEXP void GuiStackDumpAt(duint addr, duint csp);
BRIDGE_IMPEXP void GuiUpdateDumpView();
BRIDGE_IMPEXP void GuiUpdateWatchView();
BRIDGE_IMPEXP void GuiUpdateThreadView();
BRIDGE_IMPEXP void GuiUpdateMemoryView();
BRIDGE_IMPEXP void GuiAddRecentFile(const char* file);
BRIDGE_IMPEXP void GuiSetLastException(unsigned int exception);
BRIDGE_IMPEXP bool GuiGetDisassembly(duint addr, char* text);
BRIDGE_IMPEXP int GuiMenuAdd(int hMenu, const char* title);
BRIDGE_IMPEXP int GuiMenuAddEntry(int hMenu, const char* title);
BRIDGE_IMPEXP void GuiMenuAddSeparator(int hMenu);
BRIDGE_IMPEXP void GuiMenuClear(int hMenu);
BRIDGE_IMPEXP void GuiMenuRemove(int hEntryMenu);
BRIDGE_IMPEXP bool GuiSelectionGet(GUISELECTIONTYPE hWindow, SELECTIONDATA* selection);
BRIDGE_IMPEXP bool GuiSelectionSet(GUISELECTIONTYPE hWindow, const SELECTIONDATA* selection);
BRIDGE_IMPEXP bool GuiGetLineWindow(const char* title, char* text);
BRIDGE_IMPEXP void GuiAutoCompleteAddCmd(const char* cmd);
BRIDGE_IMPEXP void GuiAutoCompleteDelCmd(const char* cmd);
BRIDGE_IMPEXP void GuiAutoCompleteClearAll();
BRIDGE_IMPEXP void GuiAddStatusBarMessage(const char* msg);
BRIDGE_IMPEXP void GuiUpdateSideBar();
BRIDGE_IMPEXP void GuiRepaintTableView();
BRIDGE_IMPEXP void GuiUpdatePatches();
BRIDGE_IMPEXP void GuiUpdateCallStack();
BRIDGE_IMPEXP void GuiUpdateSEHChain();
BRIDGE_IMPEXP void GuiLoadSourceFileEx(const char* path, duint addr);
BRIDGE_IMPEXP void GuiMenuSetIcon(int hMenu, const ICONDATA* icon);
BRIDGE_IMPEXP void GuiMenuSetEntryIcon(int hEntry, const ICONDATA* icon);
BRIDGE_IMPEXP void GuiMenuSetEntryChecked(int hEntry, bool checked);
BRIDGE_IMPEXP void GuiMenuSetVisible(int hMenu, bool visible);
BRIDGE_IMPEXP void GuiMenuSetEntryVisible(int hEntry, bool visible);
BRIDGE_IMPEXP void GuiMenuSetName(int hMenu, const char* name);
BRIDGE_IMPEXP void GuiMenuSetEntryName(int hEntry, const char* name);
BRIDGE_IMPEXP void GuiMenuSetEntryHotkey(int hEntry, const char* hack);
BRIDGE_IMPEXP void GuiShowCpu();
BRIDGE_IMPEXP void GuiShowThreads();
BRIDGE_IMPEXP void GuiAddQWidgetTab(void* qWidget);
BRIDGE_IMPEXP void GuiShowQWidgetTab(void* qWidget);
BRIDGE_IMPEXP void GuiCloseQWidgetTab(void* qWidget);
BRIDGE_IMPEXP void GuiExecuteOnGuiThread(GUICALLBACK cbGuiThread);
BRIDGE_IMPEXP void GuiUpdateTimeWastedCounter();
BRIDGE_IMPEXP void GuiSetGlobalNotes(const char* text);
BRIDGE_IMPEXP void GuiGetGlobalNotes(char** text);
BRIDGE_IMPEXP void GuiSetDebuggeeNotes(const char* text);
BRIDGE_IMPEXP void GuiGetDebuggeeNotes(char** text);
BRIDGE_IMPEXP void GuiDumpAtN(duint va, int index);
BRIDGE_IMPEXP void GuiDisplayWarning(const char* title, const char* text);
BRIDGE_IMPEXP void GuiRegisterScriptLanguage(SCRIPTTYPEINFO* info);
BRIDGE_IMPEXP void GuiUnregisterScriptLanguage(int id);
BRIDGE_IMPEXP void GuiUpdateArgumentWidget();
BRIDGE_IMPEXP void GuiFocusView(int hWindow);
BRIDGE_IMPEXP bool GuiIsUpdateDisabled();
BRIDGE_IMPEXP void GuiUpdateEnable(bool updateNow);
BRIDGE_IMPEXP void GuiUpdateDisable();
BRIDGE_IMPEXP bool GuiLoadGraph(BridgeCFGraphList* graph, duint addr);
BRIDGE_IMPEXP duint GuiGraphAt(duint addr);
BRIDGE_IMPEXP void GuiUpdateGraphView();
BRIDGE_IMPEXP void GuiDisableLog();
BRIDGE_IMPEXP void GuiEnableLog();
BRIDGE_IMPEXP bool GuiIsLogEnabled();
BRIDGE_IMPEXP void GuiAddFavouriteTool(const char* name, const char* description);
BRIDGE_IMPEXP void GuiAddFavouriteCommand(const char* name, const char* shortcut);
BRIDGE_IMPEXP void GuiSetFavouriteToolShortcut(const char* name, const char* shortcut);
BRIDGE_IMPEXP void GuiFoldDisassembly(duint startAddress, duint length);
BRIDGE_IMPEXP void GuiSelectInMemoryMap(duint addr);
BRIDGE_IMPEXP void GuiGetActiveView(ACTIVEVIEW* activeView);
BRIDGE_IMPEXP void GuiAddInfoLine(const char* infoLine);
BRIDGE_IMPEXP void GuiProcessEvents();
BRIDGE_IMPEXP void* GuiTypeAddNode(void* parent, const TYPEDESCRIPTOR* type);
BRIDGE_IMPEXP bool GuiTypeClear();
BRIDGE_IMPEXP void GuiUpdateTypeWidget();
BRIDGE_IMPEXP void GuiCloseApplication();
BRIDGE_IMPEXP void GuiFlushLog();
BRIDGE_IMPEXP void GuiReferenceAddCommand(const char* title, const char* command);
BRIDGE_IMPEXP void GuiUpdateTraceBrowser();
BRIDGE_IMPEXP void GuiOpenTraceFile(const char* fileName);
BRIDGE_IMPEXP void GuiInvalidateSymbolSource(duint base);
BRIDGE_IMPEXP void GuiExecuteOnGuiThreadEx(GUICALLBACKEX cbGuiThread, void* userdata);
BRIDGE_IMPEXP void GuiGetCurrentGraph(BridgeCFGraphList* graphList);
BRIDGE_IMPEXP void GuiShowReferences();
BRIDGE_IMPEXP void GuiSelectInSymbolsTab(duint addr);
BRIDGE_IMPEXP void GuiGotoTrace(duint index);
BRIDGE_IMPEXP void GuiShowTrace();
BRIDGE_IMPEXP DWORD GuiGetMainThreadId();

#ifdef __cplusplus
}
#endif // __cplusplus

// Some useful C++ wrapper classes
#ifdef __cplusplus

class GuiDisableLogScope
{
    bool wasEnabled;

public:
    GuiDisableLogScope(const GuiDisableLogScope &) = delete;

    GuiDisableLogScope()
    {
        wasEnabled = GuiIsLogEnabled();
        if(wasEnabled)
            GuiDisableLog();
    }

    ~GuiDisableLogScope()
    {
        if(wasEnabled)
            GuiEnableLog();
    }
};

class GuiDisableUpdateScope
{
    bool updateAfter;
    bool wasEnabled;

public:
    GuiDisableUpdateScope(const GuiDisableUpdateScope &) = delete;

    explicit GuiDisableUpdateScope(bool updateAfter = true)
        : updateAfter(updateAfter)
    {
        wasEnabled = !GuiIsUpdateDisabled();
        if(wasEnabled)
            GuiUpdateDisable();
    }

    ~GuiDisableUpdateScope()
    {
        if(wasEnabled)
            GuiUpdateEnable(updateAfter);
    }
};

class GuiDisableScope : GuiDisableUpdateScope, GuiDisableLogScope { };

#endif // __cplusplus

#pragma pack(pop)

#endif // _BRIDGEMAIN_H_

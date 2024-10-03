#ifndef _DBGFUNCTIONS_H
#define _DBGFUNCTIONS_H

#ifndef __cplusplus
#include <stdbool.h>
#else
#include <string>
#endif

typedef struct
{
    char mod[MAX_MODULE_SIZE];
    duint addr;
    unsigned char oldbyte;
    unsigned char newbyte;
} DBGPATCHINFO;

typedef struct
{
    duint addr;
    duint from;
    duint to;
    char comment[MAX_COMMENT_SIZE];
} DBGCALLSTACKENTRY;

typedef struct
{
    int total;
    DBGCALLSTACKENTRY* entries;
} DBGCALLSTACK;

typedef struct
{
    duint addr;
    duint handler;
} DBGSEHRECORD;

typedef struct
{
    duint total;
    DBGSEHRECORD* records;
} DBGSEHCHAIN;

typedef struct
{
    DWORD dwProcessId;
    char szExeFile[MAX_PATH];
    char szExeMainWindowTitle[MAX_PATH];
    char szExeArgs[MAX_COMMAND_LINE_SIZE];
} DBGPROCESSINFO;

typedef struct
{
    DWORD rva;
    BYTE type;
    WORD size;
} DBGRELOCATIONINFO;

typedef enum
{
    InstructionBody = 0,
    InstructionHeading = 1,
    InstructionTailing = 2,
    InstructionOverlapped = 3, // The byte was executed with differing instruction base addresses
    DataByte,  // This and the following is not implemented yet.
    DataWord,
    DataDWord,
    DataQWord,
    DataFloat,
    DataDouble,
    DataLongDouble,
    DataXMM,
    DataYMM,
    DataMMX,
    DataMixed, //the byte is accessed in multiple ways
    InstructionDataMixed //the byte is both executed and written
} TRACERECORDBYTETYPE;

typedef enum
{
    TraceRecordNone,
    TraceRecordBitExec,
    TraceRecordByteWithExecTypeAndCounter,
    TraceRecordWordWithExecTypeAndCounter
} TRACERECORDTYPE;

typedef struct
{
    duint Handle;
    unsigned char TypeNumber;
    unsigned int GrantedAccess;
} HANDLEINFO;

// The longest ip address is 1234:6789:1234:6789:1234:6789:123.567.901.345 (46 bytes)
#define TCP_ADDR_SIZE 50

typedef struct
{
    char RemoteAddress[TCP_ADDR_SIZE];
    unsigned short RemotePort;
    char LocalAddress[TCP_ADDR_SIZE];
    unsigned short LocalPort;
    char StateText[TCP_ADDR_SIZE];
    unsigned int State;
} TCPCONNECTIONINFO;

typedef struct
{
    duint handle;
    duint parent;
    DWORD threadId;
    DWORD style;
    DWORD styleEx;
    duint wndProc;
    bool enabled;
    RECT position;
    char windowTitle[MAX_COMMENT_SIZE];
    char windowClass[MAX_COMMENT_SIZE];
} WINDOW_INFO;

typedef struct
{
    duint addr;
    duint size;
    duint flags;
} HEAPINFO;

typedef struct
{
    const char* name;
    duint value;
} CONSTANTINFO;

typedef enum
{
    MODSYMUNLOADED = 0,
    MODSYMLOADING,
    MODSYMLOADED
} MODULESYMBOLSTATUS;

typedef enum
{
    bpf_type, // number (read-only, BPXTYPE)
    bpf_offset, // number (read-only)
    bpf_address, // number (read-only)
    bpf_enabled, // number (bool)
    bpf_singleshoot, // number (bool)
    bpf_active, // number (read-only)
    bpf_silent, // number (bool)
    bpf_typeex, // number (read-only, BPHWTYPE/BPMEMTYPE/BPDLLTYPE/BPEXTYPE)
    bpf_hwsize, // number (read-only, BPHWSIZE)
    bpf_hwslot, // number (read-only)
    bpf_oldbytes, // number (read-only, uint16_t)
    bpf_fastresume, // number (bool)
    bpf_hitcount, // number
    bpf_module, // text (read-only)
    bpf_name, // text
    bpf_breakcondition, // text
    bpf_logtext, // text
    bpf_logcondition, // text
    bpf_commandtext, // text
    bpf_commandcondition, // text
    bpf_logfile, // text
} BP_FIELD;

// An instance of this structure represents a reference to a breakpoint.
// Use DbgFunctions()->BpRefXxx() list/create references.
// Use DbgFunctions()->BpXxx() to manipulate breakpoints with the references.
struct BP_REF
{
    BPXTYPE type;
    duint module;
    duint offset;

    // C++ helper functions
#ifdef __cplusplus
    bool GetField(BP_FIELD field, duint & value);
    bool GetField(BP_FIELD field, bool & value);
    bool SetField(BP_FIELD field, duint value);
    bool GetField(BP_FIELD field, std::string & value);
    bool SetField(BP_FIELD field, const std::string & value);

    template<class T, typename = typename std::enable_if< std::is_enum<T>::value, T >::type>
    void GetField(BP_FIELD field, T & value)
    {
        duint n = 0;
        GetField(field, n);
        value = (T)n;
    }
#endif // __cplusplus
};

typedef struct BP_REF BP_REF;

typedef void(*CBSTRING)(const char* str, void* userdata);

//The list of all the DbgFunctions() return value.
//WARNING: This list is append only. Do not insert things in the middle or plugins would break.
typedef struct DBGFUNCTIONS_
{
    bool (*AssembleAtEx)(duint addr, const char* instruction, char* error, bool fillnop);
    bool (*SectionFromAddr)(duint addr, char* section);
    bool (*ModNameFromAddr)(duint addr, char* modname, bool extension);
    duint(*ModBaseFromAddr)(duint addr);
    duint(*ModBaseFromName)(const char* modname);
    duint(*ModSizeFromAddr)(duint addr);
    bool (*Assemble)(duint addr, unsigned char* dest, int* size, const char* instruction, char* error);
    bool (*PatchGet)(duint addr);
    bool (*PatchInRange)(duint start, duint end);
    bool (*MemPatch)(duint va, const unsigned char* src, duint size);
    void (*PatchRestoreRange)(duint start, duint end);
    bool (*PatchEnum)(DBGPATCHINFO* patchlist, size_t* cbsize);
    bool (*PatchRestore)(duint addr);
    int (*PatchFile)(DBGPATCHINFO* patchlist, int count, const char* szFileName, char* error);
    int (*ModPathFromAddr)(duint addr, char* path, int size);
    int (*ModPathFromName)(const char* modname, char* path, int size);
    bool (*DisasmFast)(const unsigned char* data, duint addr, BASIC_INSTRUCTION_INFO* basicinfo);
    void (*MemUpdateMap)();
    void (*GetCallStack)(DBGCALLSTACK* callstack);
    void (*GetSEHChain)(DBGSEHCHAIN* sehchain);
    void (*SymbolDownloadAllSymbols)(const char* szSymbolStore);
    bool (*GetJitAuto)(bool* jitauto);
    bool (*GetJit)(char* jit, bool x64);
    bool (*GetDefJit)(char* defjit);
    bool (*GetProcessList)(DBGPROCESSINFO** entries, int* count);
    bool (*GetPageRights)(duint addr, char* rights);
    bool (*SetPageRights)(duint addr, const char* rights);
    bool (*PageRightsToString)(DWORD protect, char* rights);
    bool (*IsProcessElevated)();
    bool (*GetCmdline)(char* cmdline, size_t* cbsize);
    bool (*SetCmdline)(const char* cmdline);
    duint(*FileOffsetToVa)(const char* modname, duint offset);
    duint(*VaToFileOffset)(duint va);
    duint(*GetAddrFromLine)(const char* szSourceFile, int line, duint* displacement);
    bool (*GetSourceFromAddr)(duint addr, char* szSourceFile, int* line);
    bool (*ValFromString)(const char* string, duint* value);
    bool (*PatchGetEx)(duint addr, DBGPATCHINFO* info);
    bool (*GetBridgeBp)(BPXTYPE type, duint addr, BRIDGEBP* bp);
    bool (*StringFormatInline)(const char* format, size_t resultSize, char* result);
    void (*GetMnemonicBrief)(const char* mnem, size_t resultSize, char* result);
    unsigned int (*GetTraceRecordHitCount)(duint address);
    TRACERECORDBYTETYPE(*GetTraceRecordByteType)(duint address);
    bool (*SetTraceRecordType)(duint pageAddress, TRACERECORDTYPE type);
    TRACERECORDTYPE(*GetTraceRecordType)(duint pageAddress);
    bool (*EnumHandles)(ListOf(HANDLEINFO) handles);
    bool (*GetHandleName)(duint handle, char* name, size_t nameSize, char* typeName, size_t typeNameSize);
    bool (*EnumTcpConnections)(ListOf(TCPCONNECTIONINFO) connections);
    duint(*GetDbgEvents)();
    MODULEPARTY(*ModGetParty)(duint base);
    void (*ModSetParty)(duint base, MODULEPARTY party);
    bool (*WatchIsWatchdogTriggered)(unsigned int id);
    bool (*MemIsCodePage)(duint addr, bool refresh);
    bool (*AnimateCommand)(const char* command);
    void (*DbgSetDebuggeeInitScript)(const char* fileName);
    const char* (*DbgGetDebuggeeInitScript)();
    bool (*EnumWindows)(ListOf(WINDOW_INFO) windows);
    bool (*EnumHeaps)(ListOf(HEAPINFO) heaps);
    bool (*ThreadGetName)(DWORD tid, char* name);
    bool (*IsDepEnabled)();
    void (*GetCallStackEx)(DBGCALLSTACK* callstack, bool cache);
    bool (*GetUserComment)(duint addr, char* comment);
    void (*EnumConstants)(ListOf(CONSTANTINFO) constants);
    void (*EnumErrorCodes)(ListOf(CONSTANTINFO) constants);
    void (*EnumExceptions)(ListOf(CONSTANTINFO) constants);
    duint(*MemBpSize)(duint addr);
    bool (*ModRelocationsFromAddr)(duint addr, ListOf(DBGRELOCATIONINFO) relocations);
    bool (*ModRelocationAtAddr)(duint addr, DBGRELOCATIONINFO* relocation);
    bool (*ModRelocationsInRange)(duint addr, duint size, ListOf(DBGRELOCATIONINFO) relocations);
    duint(*DbGetHash)();
    int (*SymAutoComplete)(const char* Search, char** Buffer, int MaxSymbols);
    void (*RefreshModuleList)();
    duint(*GetAddrFromLineEx)(duint mod, const char* szSourceFile, int line);
    MODULESYMBOLSTATUS(*ModSymbolStatus)(duint mod);
    void (*GetCallStackByThread)(HANDLE thread, DBGCALLSTACK* callstack);
    void (*EnumStructs)(CBSTRING callback, void* userdata);
    BP_REF* (*BpRefList)(duint* count);
    bool (*BpRefVa)(BP_REF* ref, BPXTYPE type, duint va);
    bool (*BpRefRva)(BP_REF* ref, BPXTYPE type, const char* module, duint rva);
    void (*BpRefDll)(BP_REF* ref, const char* module);
    void (*BpRefException)(BP_REF* ref, unsigned int code);
    bool (*BpRefExists)(const BP_REF* ref);
    bool (*BpGetFieldNumber)(const BP_REF* ref, BP_FIELD field, duint* value);
    bool (*BpSetFieldNumber)(const BP_REF* ref, BP_FIELD field, duint value);
    bool (*BpGetFieldText)(const BP_REF* ref, BP_FIELD field, CBSTRING callback, void* userdata);
    bool (*BpSetFieldText)(const BP_REF* ref, BP_FIELD field, const char* value);
} DBGFUNCTIONS;

#ifdef __cplusplus
inline bool BP_REF::GetField(BP_FIELD field, duint & value)
{
    return DbgFunctions()->BpGetFieldNumber(this, field, &value);
}

inline bool BP_REF::GetField(BP_FIELD field, bool & value)
{
    duint n = 0;
    if(!DbgFunctions()->BpGetFieldNumber(this, field, &n))
        return false;
    value = !!n;
    return true;
}

inline bool BP_REF::SetField(BP_FIELD field, duint value)
{
    return DbgFunctions()->BpSetFieldNumber(this, field, value);
}

inline bool BP_REF::GetField(BP_FIELD field, std::string & value)
{
    return DbgFunctions()->BpGetFieldText(this, field, [](const char* str, void* userdata)
    {
        *(std::string*)userdata = str;
    }, &value);
}

inline bool BP_REF::SetField(BP_FIELD field, const std::string & value)
{
    return DbgFunctions()->BpSetFieldText(this, field, value.c_str());
}
#endif // __cplusplus

#ifdef BUILD_DBG

const DBGFUNCTIONS* dbgfunctionsget();
void dbgfunctionsinit();

#endif //BUILD_DBG

#endif //_DBGFUNCTIONS_H

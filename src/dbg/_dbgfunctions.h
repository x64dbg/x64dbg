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
typedef struct
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
        getField(field, n);
        value = (T)n;
    }
#endif // __cplusplus
} BP_REF;

typedef void(*CBSTRING)(const char* str, void* userdata);

typedef bool (*ASSEMBLEATEX)(duint addr, const char* instruction, char* error, bool fillnop);
typedef bool (*SECTIONFROMADDR)(duint addr, char* section);
typedef bool (*MODNAMEFROMADDR)(duint addr, char* modname, bool extension);
typedef duint(*MODBASEFROMADDR)(duint addr);
typedef duint(*MODBASEFROMNAME)(const char* modname);
typedef duint(*MODSIZEFROMADDR)(duint addr);
typedef bool (*ASSEMBLE)(duint addr, unsigned char* dest, int* size, const char* instruction, char* error);
typedef bool (*PATCHGET)(duint addr);
typedef bool (*PATCHINRANGE)(duint start, duint end);
typedef bool (*MEMPATCH)(duint va, const unsigned char* src, duint size);
typedef void (*PATCHRESTORERANGE)(duint start, duint end);
typedef bool (*PATCHENUM)(DBGPATCHINFO* patchlist, size_t* cbsize);
typedef bool (*PATCHRESTORE)(duint addr);
typedef int (*PATCHFILE)(DBGPATCHINFO* patchlist, int count, const char* szFileName, char* error);
typedef int (*MODPATHFROMADDR)(duint addr, char* path, int size);
typedef int (*MODPATHFROMNAME)(const char* modname, char* path, int size);
typedef bool (*DISASMFAST)(const unsigned char* data, duint addr, BASIC_INSTRUCTION_INFO* basicinfo);
typedef void (*MEMUPDATEMAP)();
typedef void (*GETCALLSTACK)(DBGCALLSTACK* callstack);
typedef void (*GETSEHCHAIN)(DBGSEHCHAIN* sehchain);
typedef void (*SYMBOLDOWNLOADALLSYMBOLS)(const char* szSymbolStore);
typedef bool (*GETJIT)(char* jit, bool x64);
typedef bool (*GETJITAUTO)(bool* jitauto);
typedef bool (*GETDEFJIT)(char* defjit);
typedef bool (*GETPROCESSLIST)(DBGPROCESSINFO** entries, int* count);
typedef bool (*GETPAGERIGHTS)(duint addr, char* rights);
typedef bool (*SETPAGERIGHTS)(duint addr, const char* rights);
typedef bool (*PAGERIGHTSTOSTRING)(DWORD protect, char* rights);
typedef bool (*ISPROCESSELEVATED)();
typedef bool (*GETCMDLINE)(char* cmdline, size_t* cbsize);
typedef bool (*SETCMDLINE)(const char* cmdline);
typedef duint(*FILEOFFSETTOVA)(const char* modname, duint offset);
typedef duint(*VATOFILEOFFSET)(duint va);
typedef duint(*GETADDRFROMLINE)(const char* szSourceFile, int line, duint* displacement);
typedef bool (*GETSOURCEFROMADDR)(duint addr, char* szSourceFile, int* line);
typedef bool (*VALFROMSTRING)(const char* string, duint* value);
typedef bool (*PATCHGETEX)(duint addr, DBGPATCHINFO* info);
typedef bool (*GETBRIDGEBP)(BPXTYPE type, duint addr, BRIDGEBP* bp);
typedef bool (*STRINGFORMATINLINE)(const char* format, size_t resultSize, char* result);
typedef void (*GETMNEMONICBRIEF)(const char* mnem, size_t resultSize, char* result);
typedef unsigned int (*GETTRACERECORDHITCOUNT)(duint address);
typedef TRACERECORDBYTETYPE(*GETTRACERECORDBYTETYPE)(duint address);
typedef bool (*SETTRACERECORDTYPE)(duint pageAddress, TRACERECORDTYPE type);
typedef TRACERECORDTYPE(*GETTRACERECORDTYPE)(duint pageAddress);
typedef bool (*ENUMHANDLES)(ListOf(HANDLEINFO) handles);
typedef bool (*GETHANDLENAME)(duint handle, char* name, size_t nameSize, char* typeName, size_t typeNameSize);
typedef bool (*ENUMTCPCONNECTIONS)(ListOf(TCPCONNECTIONINFO) connections);
typedef duint(*GETDBGEVENTS)();
typedef MODULEPARTY(*MODGETPARTY)(duint base);
typedef void (*MODSETPARTY)(duint base, MODULEPARTY party);
typedef bool(*WATCHISWATCHDOGTRIGGERED)(unsigned int id);
typedef bool(*MEMISCODEPAGE)(duint addr, bool refresh);
typedef bool(*ANIMATECOMMAND)(const char* command);
typedef void(*DBGSETDEBUGGEEINITSCRIPT)(const char* fileName);
typedef const char* (*DBGGETDEBUGGEEINITSCRIPT)();
typedef bool(*HANDLESENUMWINDOWS)(ListOf(WINDOW_INFO) windows);
typedef bool(*HANDLESENUMHEAPS)(ListOf(HEAPINFO) heaps);
typedef bool(*THREADGETNAME)(DWORD tid, char* name);
typedef bool(*ISDEPENABLED)();
typedef void(*GETCALLSTACKEX)(DBGCALLSTACK* callstack, bool cache);
typedef bool(*GETUSERCOMMENT)(duint addr, char* comment);
typedef void(*ENUMCONSTANTS)(ListOf(CONSTANTINFO) constants);
typedef duint(*MEMBPSIZE)(duint addr);
typedef bool(*MODRELOCATIONSFROMADDR)(duint addr, ListOf(DBGRELOCATIONINFO) relocations);
typedef bool(*MODRELOCATIONATADDR)(duint addr, DBGRELOCATIONINFO* relocation);
typedef bool(*MODRELOCATIONSINRANGE)(duint addr, duint size, ListOf(DBGRELOCATIONINFO) relocations);
typedef duint(*DBGETHASH)();
typedef int(*SYMAUTOCOMPLETE)(const char* Search, char** Buffer, int MaxSymbols);
typedef void(*REFRESHMODULELIST)();
typedef duint(*GETADDRFROMLINEEX)(duint mod, const char* szSourceFile, int line);
typedef MODULESYMBOLSTATUS(*MODSYMBOLSTATUS)(duint mod);
typedef void(*GETCALLSTACKBYTHREAD)(HANDLE thread, DBGCALLSTACK* callstack);

//The list of all the DbgFunctions() return value.
//WARNING: This list is append only. Do not insert things in the middle or plugins would break.
typedef struct DBGFUNCTIONS_
{
    ASSEMBLEATEX AssembleAtEx;
    SECTIONFROMADDR SectionFromAddr;
    MODNAMEFROMADDR ModNameFromAddr;
    MODBASEFROMADDR ModBaseFromAddr;
    MODBASEFROMNAME ModBaseFromName;
    MODSIZEFROMADDR ModSizeFromAddr;
    ASSEMBLE Assemble;
    PATCHGET PatchGet;
    PATCHINRANGE PatchInRange;
    MEMPATCH MemPatch;
    PATCHRESTORERANGE PatchRestoreRange;
    PATCHENUM PatchEnum;
    PATCHRESTORE PatchRestore;
    PATCHFILE PatchFile;
    MODPATHFROMADDR ModPathFromAddr;
    MODPATHFROMNAME ModPathFromName;
    DISASMFAST DisasmFast;
    MEMUPDATEMAP MemUpdateMap;
    GETCALLSTACK GetCallStack;
    GETSEHCHAIN GetSEHChain;
    SYMBOLDOWNLOADALLSYMBOLS SymbolDownloadAllSymbols;
    GETJITAUTO GetJitAuto;
    GETJIT GetJit;
    GETDEFJIT GetDefJit;
    GETPROCESSLIST GetProcessList;
    GETPAGERIGHTS GetPageRights;
    SETPAGERIGHTS SetPageRights;
    PAGERIGHTSTOSTRING PageRightsToString;
    ISPROCESSELEVATED IsProcessElevated;
    GETCMDLINE GetCmdline;
    SETCMDLINE SetCmdline;
    FILEOFFSETTOVA FileOffsetToVa;
    VATOFILEOFFSET VaToFileOffset;
    GETADDRFROMLINE GetAddrFromLine;
    GETSOURCEFROMADDR GetSourceFromAddr;
    VALFROMSTRING ValFromString;
    PATCHGETEX PatchGetEx;
    GETBRIDGEBP GetBridgeBp;
    STRINGFORMATINLINE StringFormatInline;
    GETMNEMONICBRIEF GetMnemonicBrief;
    GETTRACERECORDHITCOUNT GetTraceRecordHitCount;
    GETTRACERECORDBYTETYPE GetTraceRecordByteType;
    SETTRACERECORDTYPE SetTraceRecordType;
    GETTRACERECORDTYPE GetTraceRecordType;
    ENUMHANDLES EnumHandles;
    GETHANDLENAME GetHandleName;
    ENUMTCPCONNECTIONS EnumTcpConnections;
    GETDBGEVENTS GetDbgEvents;
    MODGETPARTY ModGetParty;
    MODSETPARTY ModSetParty;
    WATCHISWATCHDOGTRIGGERED WatchIsWatchdogTriggered;
    MEMISCODEPAGE MemIsCodePage;
    ANIMATECOMMAND AnimateCommand;
    DBGSETDEBUGGEEINITSCRIPT DbgSetDebuggeeInitScript;
    DBGGETDEBUGGEEINITSCRIPT DbgGetDebuggeeInitScript;
    HANDLESENUMWINDOWS EnumWindows;
    HANDLESENUMHEAPS EnumHeaps;
    THREADGETNAME ThreadGetName;
    ISDEPENABLED IsDepEnabled;
    GETCALLSTACKEX GetCallStackEx;
    GETUSERCOMMENT GetUserComment;
    ENUMCONSTANTS EnumConstants;
    ENUMCONSTANTS EnumErrorCodes;
    ENUMCONSTANTS EnumExceptions;
    MEMBPSIZE MemBpSize;
    MODRELOCATIONSFROMADDR ModRelocationsFromAddr;
    MODRELOCATIONATADDR ModRelocationAtAddr;
    MODRELOCATIONSINRANGE ModRelocationsInRange;
    DBGETHASH DbGetHash;
    SYMAUTOCOMPLETE SymAutoComplete;
    REFRESHMODULELIST RefreshModuleList;
    GETADDRFROMLINEEX GetAddrFromLineEx;
    MODSYMBOLSTATUS ModSymbolStatus;
    GETCALLSTACKBYTHREAD GetCallStackByThread;
    void (*EnumStructs)(CBSTRING callback, void* userdata);
    // New Breakpoint API
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

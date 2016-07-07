#ifndef _DBGFUNCTIONS_H
#define _DBGFUNCTIONS_H

#ifndef __cplusplus
#include <stdbool.h>
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
} DBGPROCESSINFO;

enum TRACERECORDBYTETYPE
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
};

enum TRACERECORDTYPE
{
    TraceRecordNone,
    TraceRecordBitExec,
    TraceRecordByteWithExecTypeAndCounter,
    TraceRecordWordWithExecTypeAndCounter
};

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
typedef duint(*GETADDRFROMLINE)(const char* szSourceFile, int line);
typedef bool (*GETSOURCEFROMADDR)(duint addr, char* szSourceFile, int* line);
typedef bool (*VALFROMSTRING)(const char* string, duint* value);
typedef bool (*PATCHGETEX)(duint addr, DBGPATCHINFO* info);
typedef bool(*GETBRIDGEBP)(BPXTYPE type, duint addr, BRIDGEBP* bp);
typedef bool(*STRINGFORMATINLINE)(const char* format, size_t resultSize, char* result);
typedef void(*GETMNEMONICBRIEF)(const char* mnem, size_t resultSize, char* result);
typedef unsigned int (*GETTRACERECORDHITCOUNT)(duint address);
typedef TRACERECORDBYTETYPE(*GETTRACERECORDBYTETYPE)(duint address);
typedef bool (*SETTRACERECORDTYPE)(duint pageAddress, TRACERECORDTYPE type);
typedef TRACERECORDTYPE(*GETTRACERECORDTYPE)(duint pageAddress);
typedef bool(*ENUMHANDLES)(ListOf(HANDLEINFO) handles);
typedef bool(*GETHANDLENAME)(duint handle, char* name, size_t nameSize, char* typeName, size_t typeNameSize);
typedef bool(*ENUMTCPCONNECTIONS)(ListOf(TCPCONNECTIONINFO) connections);
typedef duint(*GETDBGEVENTS)();
typedef int(*MODGETPARTY)(duint base);
typedef void(*MODSETPARTY)(duint base, int party);
typedef bool(*WATCHISWATCHDOGTRIGGERED)(unsigned int id);

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
} DBGFUNCTIONS;

#ifdef BUILD_DBG

const DBGFUNCTIONS* dbgfunctionsget();
void dbgfunctionsinit();

#endif //BUILD_DBG

#endif //_DBGFUNCTIONS_H

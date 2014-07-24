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

typedef bool (*ASSEMBLEATEX)(duint addr, const char* instruction, char* error, bool fillnop);
typedef bool (*SECTIONFROMADDR)(duint addr, char* section);
typedef bool (*MODNAMEFROMADDR)(duint addr, char* modname, bool extension);
typedef duint (*MODBASEFROMADDR)(duint addr);
typedef duint (*MODBASEFROMNAME)(const char* modname);
typedef duint (*MODSIZEFROMADDR)(duint addr);
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
typedef bool (*DISASMFAST)(unsigned char* data, duint addr, BASIC_INSTRUCTION_INFO* basicinfo);
typedef void (*MEMUPDATEMAP)(HANDLE hProcess);
typedef void (*GETCALLSTACK)(DBGCALLSTACK* callstack);

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
} DBGFUNCTIONS;

#ifdef BUILD_DBG

const DBGFUNCTIONS* dbgfunctionsget();
void dbgfunctionsinit();

#endif //BUILD_DBG

#endif //_DBGFUNCTIONS_H

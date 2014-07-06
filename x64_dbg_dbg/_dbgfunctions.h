#ifndef _DBGFUNCTIONS_H
#define _DBGFUNCTIONS_H

struct DBGPATCHINFO
{
    char mod[MAX_MODULE_SIZE];
    duint addr;
    unsigned char oldbyte;
    unsigned char newbyte;
};

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
typedef bool (*PATCHENUM)(DBGPATCHINFO* patchinfo, size_t* cbsize);
typedef bool (*PATCHRESTORE)(duint addr);

struct DBGFUNCTIONS
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
};

#ifdef BUILD_DBG

const DBGFUNCTIONS* dbgfunctionsget();
void dbgfunctionsinit();

#endif //BUILD_DBG

#endif //_DBGFUNCTIONS_H

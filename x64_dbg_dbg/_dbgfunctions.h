#ifndef _DBGFUNCTIONS_H
#define _DBGFUNCTIONS_H

typedef bool (*ASSEMBLEATEX)(duint addr, const char* instruction, char* error, bool fillnop);
typedef bool (*SECTIONFROMADDR)(duint addr, char* section);
typedef bool (*MODNAMEFROMADDR)(uint addr, char* modname, bool extension);
typedef duint (*MODBASEFROMADDR)(uint addr);
typedef duint (*MODBASEFROMNAME)(const char* modname);
typedef duint (*MODSIZEFROMADDR)(uint addr);
typedef bool (*ASSEMBLE)(uint addr, unsigned char* dest, int* size, const char* instruction, char* error);

struct DBGFUNCTIONS
{
    ASSEMBLEATEX AssembleAtEx;
    SECTIONFROMADDR SectionFromAddr;
    MODNAMEFROMADDR ModNameFromAddr;
    MODBASEFROMADDR ModBaseFromAddr;
    MODBASEFROMNAME ModBaseFromName;
    MODSIZEFROMADDR ModSizeFromAddr;
    ASSEMBLE Assemble;
};

#ifdef BUILD_DBG

const DBGFUNCTIONS* dbgfunctionsget();
void dbgfunctionsinit();

#endif //BUILD_DBG

#endif //_DBGFUNCTIONS_H

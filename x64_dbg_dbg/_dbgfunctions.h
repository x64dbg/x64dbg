#ifndef _DBGFUNCTIONS_H
#define _DBGFUNCTIONS_H

typedef bool (*DBGASSEMBLEATEX)(duint addr, const char* instruction, char* error, bool fillnop);
typedef bool (*DBGSECTIONFROMADDR)(duint addr, char* section);

struct DBGFUNCTIONS
{
    DBGASSEMBLEATEX DbgAssembleAtEx;
    DBGSECTIONFROMADDR DbgSectionFromAddr;
};

#ifdef BUILD_DBG

const DBGFUNCTIONS* dbgfunctionsget();
void dbgfunctionsinit();

#endif //BUILD_DBG

#endif //_DBGFUNCTIONS_H

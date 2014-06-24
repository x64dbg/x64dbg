#ifndef _DBGFUNCTIONS_H
#define _DBGFUNCTIONS_H

typedef bool (*DBGASSEMBLEATEX)(duint addr, const char* instruction, char* error, bool fillnop);

struct DBGFUNCTIONS
{
    DBGASSEMBLEATEX DbgAssembleAtEx;
};

#ifdef BUILD_DBG

const DBGFUNCTIONS* dbgfunctionsget();
void dbgfunctionsinit();

#endif //BUILD_DBG

#endif //_DBGFUNCTIONS_H

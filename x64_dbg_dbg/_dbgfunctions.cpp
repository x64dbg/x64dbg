#include "_global.h"
#include "_dbgfunctions.h"
#include "assemble.h"

static DBGFUNCTIONS _dbgfunctions;

const DBGFUNCTIONS* dbgfunctionsget()
{
    return &_dbgfunctions;
}

void dbgfunctionsinit()
{
    _dbgfunctions.DbgAssembleAtEx=assembleat;
}
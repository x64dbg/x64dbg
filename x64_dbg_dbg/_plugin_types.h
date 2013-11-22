#ifndef _PLUGIN_DATA_H
#define _PLUGIN_DATA_H

#ifdef BUILD_DBG

#include "_global.h"

#else

#ifdef __GNUC__
#include "dbghelp\dbghelp.h"
#else
#include <dbghelp.h>
#endif //__GNUC__

#define deflen 1024

#ifdef _WIN64 //defined by default
#define fhex "%.16llX"
#define fext "ll"
typedef unsigned long long duint;
typedef long long dsint;
#else
#define fhex "%.8X"
#define fext ""
typedef unsigned long duint;
typedef long dsint;
#endif // _WIN64

#endif //BUILD_DBG

#endif // _PLUGIN_DATA_H

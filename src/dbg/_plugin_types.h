#ifndef _PLUGIN_DATA_H
#define _PLUGIN_DATA_H

#ifdef BUILD_DBG

#include "_global.h"

#else

#ifdef __GNUC__
#include "dbghelp\dbghelp.h"
#else
#include <dbghelp.h>
#endif // __GNUC__

#ifndef deflen
#define deflen 1024
#endif // deflen

#include "bridgemain.h"
#include "_dbgfunctions.h"

#endif // BUILD_DBG

#endif // _PLUGIN_DATA_H

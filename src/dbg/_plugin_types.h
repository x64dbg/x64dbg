#ifndef _PLUGIN_DATA_H
#define _PLUGIN_DATA_H

#ifdef BUILD_DBG

#include "_global.h"
#include "jansson/jansson.h"
#pragma warning(push)
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)

#else

#ifdef __GNUC__
#include "dbghelp/dbghelp.h"
#else
#pragma warning(push)
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)
#endif // __GNUC__

#ifndef deflen
#define deflen 1024
#endif // deflen

#include "bridgemain.h"
#include "_dbgfunctions.h"
#include "jansson/jansson.h"

#endif // BUILD_DBG

#endif // _PLUGIN_DATA_H

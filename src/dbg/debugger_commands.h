#ifndef _DEBUGGER_COMMANDS_H
#define _DEBUGGER_COMMANDS_H

#include "command.h"
#include "debugger.h"

//command callbacks
CMDRESULT cbDebugRunInternal(int argc, char* argv[]);

//misc
void showcommandlineerror(cmdline_error_t* cmdline_error);

#endif //_DEBUGGER_COMMANDS_H

#ifndef _COMMANDLINE_H
#define _COMMANDLINE_H

#include "_global.h"
#include "command.h"
#include "debugger.h"

void showcommandlineerror(cmdline_error_t* cmdline_error);
bool isCmdLineEmpty();
char* getCommandLineArgs();
void CmdLineCacheSave(JSON Root);
void CmdLineCacheLoad(JSON Root);
void copyCommandLine(const char* cmdLine);
bool setCommandLine();

#endif // _COMMANDLINE_H
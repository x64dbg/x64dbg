#ifndef _COMMANDLINE_H
#define _COMMANDLINE_H

#include "_global.h"
#include "command.h"

bool isCmdLineEmpty();
char* getCommandLineArgs();
void CmdLineCacheSave(JSON Root);
void CmdLineCacheLoad(JSON Root);
void copyCommandLine(const char* cmdLine);
CMDRESULT setCommandLine();

#endif // _COMMANDLINE_H
#pragma once

#include "_global.h"
#include "command.h"

bool isCmdLineEmpty();
char* getCommandLineArgs();
void CmdLineCacheSave(JSON Root);
void CmdLineCacheLoad(JSON Root);
void copyCommandLine(const char* cmdLine);
CMDRESULT setCommandLine();
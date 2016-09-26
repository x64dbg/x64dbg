#pragma once

#include "command.h"

CMDRESULT cbDebugAlloc(int argc, char* argv[]);
CMDRESULT cbDebugFree(int argc, char* argv[]);
CMDRESULT cbDebugMemset(int argc, char* argv[]);
CMDRESULT cbDebugGetPageRights(int argc, char* argv[]);
CMDRESULT cbDebugSetPageRights(int argc, char* argv[]);
CMDRESULT cbInstrSavedata(int argc, char* argv[]);
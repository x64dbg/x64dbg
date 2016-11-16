#pragma once

#include "command.h"

bool cbDebugAlloc(int argc, char* argv[]);
bool cbDebugFree(int argc, char* argv[]);
bool cbDebugMemset(int argc, char* argv[]);
bool cbDebugGetPageRights(int argc, char* argv[]);
bool cbDebugSetPageRights(int argc, char* argv[]);
bool cbInstrSavedata(int argc, char* argv[]);
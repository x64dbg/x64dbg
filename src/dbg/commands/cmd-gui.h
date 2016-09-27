#pragma once

#include "command.h"

CMDRESULT cbDebugDisasm(int argc, char* argv[]);
CMDRESULT cbDebugDump(int argc, char* argv[]);
CMDRESULT cbDebugStackDump(int argc, char* argv[]);
CMDRESULT cbDebugMemmapdump(int argc, char* argv[]);
CMDRESULT cbInstrGraph(int argc, char* argv[]);
CMDRESULT cbInstrEnableGuiUpdate(int argc, char* argv[]);
CMDRESULT cbInstrDisableGuiUpdate(int argc, char* argv[]);
CMDRESULT cbDebugSetfreezestack(int argc, char* argv[]);
CMDRESULT cbInstrRefinit(int argc, char* argv[]);
CMDRESULT cbInstrRefadd(int argc, char* argv[]);
CMDRESULT cbInstrEnableLog(int argc, char* argv[]);
CMDRESULT cbInstrDisableLog(int argc, char* argv[]);
CMDRESULT cbInstrAddFavTool(int argc, char* argv[]);
CMDRESULT cbInstrAddFavCmd(int argc, char* argv[]);
CMDRESULT cbInstrSetFavToolShortcut(int argc, char* argv[]);
CMDRESULT cbInstrFoldDisassembly(int argc, char* argv[]);

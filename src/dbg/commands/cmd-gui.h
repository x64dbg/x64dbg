#pragma once

#include "command.h"

bool cbDebugDisasm(int argc, char* argv[]);
bool cbDebugDump(int argc, char* argv[]);
bool cbDebugStackDump(int argc, char* argv[]);
bool cbDebugMemmapdump(int argc, char* argv[]);
bool cbInstrGraph(int argc, char* argv[]);
bool cbInstrEnableGuiUpdate(int argc, char* argv[]);
bool cbInstrDisableGuiUpdate(int argc, char* argv[]);
bool cbDebugSetfreezestack(int argc, char* argv[]);
bool cbInstrRefinit(int argc, char* argv[]);
bool cbInstrRefadd(int argc, char* argv[]);
bool cbInstrRefGet(int argc, char* argv[]);
bool cbInstrEnableLog(int argc, char* argv[]);
bool cbInstrDisableLog(int argc, char* argv[]);
bool cbInstrAddFavTool(int argc, char* argv[]);
bool cbInstrAddFavCmd(int argc, char* argv[]);
bool cbInstrSetFavToolShortcut(int argc, char* argv[]);
bool cbInstrFoldDisassembly(int argc, char* argv[]);
bool cbDebugUpdateTitle(int argc, char* argv[]);
bool cbShowReferences(int argc, char* argv[]);

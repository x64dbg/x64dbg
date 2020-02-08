#pragma once

#include "command.h"

bool cbBadCmd(int argc, char* argv[]);
bool cbDebugBenchmark(int argc, char* argv[]);
bool cbInstrSetstr(int argc, char* argv[]);
bool cbInstrGetstr(int argc, char* argv[]);
bool cbInstrCopystr(int argc, char* argv[]);
bool cbInstrZydis(int argc, char* argv[]);
bool cbInstrVisualize(int argc, char* argv[]);
bool cbInstrMeminfo(int argc, char* argv[]);
bool cbInstrBriefcheck(int argc, char* argv[]);
bool cbInstrFocusinfo(int argc, char* argv[]);
bool cbInstrFlushlog(int argc, char* argv[]);
bool cbInstrAnimateWait(int argc, char* argv[]);
bool cbInstrCoroutine(int argc, char* argv[]);

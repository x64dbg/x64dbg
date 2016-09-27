#pragma once

#include "command.h"

CMDRESULT cbBadCmd(int argc, char* argv[]);
CMDRESULT cbDebugBenchmark(int argc, char* argv[]);
CMDRESULT cbInstrSetstr(int argc, char* argv[]);
CMDRESULT cbInstrGetstr(int argc, char* argv[]);
CMDRESULT cbInstrCopystr(int argc, char* argv[]);
CMDRESULT cbInstrLoopList(int argc, char* argv[]);
CMDRESULT cbInstrCapstone(int argc, char* argv[]);
CMDRESULT cbInstrVisualize(int argc, char* argv[]);
CMDRESULT cbInstrMeminfo(int argc, char* argv[]);
CMDRESULT cbInstrBriefcheck(int argc, char* argv[]);
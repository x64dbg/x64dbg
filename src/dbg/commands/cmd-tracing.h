#pragma once

#include "command.h"

CMDRESULT cbDebugTraceIntoConditional(int argc, char* argv[]);
CMDRESULT cbDebugTraceOverConditional(int argc, char* argv[]);
CMDRESULT cbDebugTraceIntoBeyondTraceRecord(int argc, char* argv[]);
CMDRESULT cbDebugTraceOverBeyondTraceRecord(int argc, char* argv[]);
CMDRESULT cbDebugTraceIntoIntoTraceRecord(int argc, char* argv[]);
CMDRESULT cbDebugTraceOverIntoTraceRecord(int argc, char* argv[]);
CMDRESULT cbDebugRunToParty(int argc, char* argv[]);
CMDRESULT cbDebugRunToUserCode(int argc, char* argv[]);
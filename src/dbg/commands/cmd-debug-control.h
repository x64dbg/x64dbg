#pragma once

#include "command.h"

CMDRESULT cbDebugRunInternal(int argc, char* argv[]);
CMDRESULT cbDebugInit(int argc, char* argv[]);
CMDRESULT cbDebugStop(int argc, char* argv[]);
CMDRESULT cbDebugAttach(int argc, char* argv[]);
CMDRESULT cbDebugDetach(int argc, char* argv[]);
CMDRESULT cbDebugRun(int argc, char* argv[]);
CMDRESULT cbDebugErun(int argc, char* argv[]);
CMDRESULT cbDebugSerun(int argc, char* argv[]);
CMDRESULT cbDebugPause(int argc, char* argv[]);
CMDRESULT cbDebugContinue(int argc, char* argv[]);
CMDRESULT cbDebugStepInto(int argc, char* argv[]);
CMDRESULT cbDebugeStepInto(int argc, char* argv[]);
CMDRESULT cbDebugseStepInto(int argc, char* argv[]);
CMDRESULT cbDebugStepOver(int argc, char* argv[]);
CMDRESULT cbDebugeStepOver(int argc, char* argv[]);
CMDRESULT cbDebugseStepOver(int argc, char* argv[]);
CMDRESULT cbDebugSingleStep(int argc, char* argv[]);
CMDRESULT cbDebugeSingleStep(int argc, char* argv[]);
CMDRESULT cbDebugStepOut(int argc, char* argv[]);
CMDRESULT cbDebugeStepOut(int argc, char* argv[]);
CMDRESULT cbDebugSkip(int argc, char* argv[]);
CMDRESULT cbInstrInstrUndo(int argc, char* argv[]);
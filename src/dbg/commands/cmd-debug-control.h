#pragma once

#include "command.h"

enum HistoryAction
{
    history_clear,
    history_record,
};

bool cbDebugRunInternal(int argc, char* argv[], HistoryAction history);
bool cbDebugInit(int argc, char* argv[]);
bool cbDebugStop(int argc, char* argv[]);
bool cbDebugAttach(int argc, char* argv[]);
bool cbDebugDetach(int argc, char* argv[]);
bool cbDebugRun(int argc, char* argv[]);
bool cbDebugErun(int argc, char* argv[]);
bool cbDebugSerun(int argc, char* argv[]);
bool cbDebugPause(int argc, char* argv[]);
bool cbDebugContinue(int argc, char* argv[]);
bool cbDebugStepInto(int argc, char* argv[]);
bool cbDebugeStepInto(int argc, char* argv[]);
bool cbDebugseStepInto(int argc, char* argv[]);
bool cbDebugStepOver(int argc, char* argv[]);
bool cbDebugeStepOver(int argc, char* argv[]);
bool cbDebugseStepOver(int argc, char* argv[]);
bool cbDebugStepOut(int argc, char* argv[]);
bool cbDebugeStepOut(int argc, char* argv[]);
bool cbDebugSkip(int argc, char* argv[]);
bool cbInstrInstrUndo(int argc, char* argv[]);
bool cbDebugStepUserInto(int argc, char* argv[]);
bool cbDebugStepSystemInto(int argc, char* argv[]);
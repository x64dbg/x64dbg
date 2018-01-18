#pragma once

#include "command.h"

bool cbDebugCreatethread(int argc, char* argv[]);
bool cbDebugSwitchthread(int argc, char* argv[]);
bool cbDebugSuspendthread(int argc, char* argv[]);
bool cbDebugResumethread(int argc, char* argv[]);
bool cbDebugKillthread(int argc, char* argv[]);
bool cbDebugSuspendAllThreads(int argc, char* argv[]);
bool cbDebugResumeAllThreads(int argc, char* argv[]);
bool cbDebugSetPriority(int argc, char* argv[]);
bool cbDebugSetthreadname(int argc, char* argv[]);
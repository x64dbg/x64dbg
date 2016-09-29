#pragma once

#include "command.h"

CMDRESULT cbDebugCreatethread(int argc, char* argv[]);
CMDRESULT cbDebugSwitchthread(int argc, char* argv[]);
CMDRESULT cbDebugSuspendthread(int argc, char* argv[]);
CMDRESULT cbDebugResumethread(int argc, char* argv[]);
CMDRESULT cbDebugKillthread(int argc, char* argv[]);
CMDRESULT cbDebugSuspendAllThreads(int argc, char* argv[]);
CMDRESULT cbDebugResumeAllThreads(int argc, char* argv[]);
CMDRESULT cbDebugSetPriority(int argc, char* argv[]);
CMDRESULT cbDebugSetthreadname(int argc, char* argv[]);
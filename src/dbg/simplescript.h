#pragma once

#include "_global.h"

enum SCRIPTSTATE
{
    SCRIPT_PAUSED = 0,
    SCRIPT_STEPPING,
    SCRIPT_RUNNING,
};

enum class ScriptInterrupt
{
    None = 0,
    YieldDebugEvent,
    AbortUser,
    AbortAssertion,
    AbortShutdown,
};

struct ScriptInterruptState
{
    SCRIPTSTATE state;
    bool gui;
};

enum class ScriptCommandOutcome
{
    Continue = 0,
    Pause,
    Abort,
};

bool ScriptLoadAwait(const char* filename, bool gui);
void ScriptUnloadAwait();
void ScriptRunAsync(int destline, bool gui);
bool ScriptRunAwait(int destline, bool gui);
void ScriptStepAsync(bool gui);
bool ScriptBpGetLocked(int line);
bool ScriptBpToggleLocked(int line);
ScriptCommandOutcome ScriptCmdExecAwait(const char* command, bool gui, const SCRIPTSTATE* interruptState);
void ScriptSetIpAwait(int line);
ScriptInterruptState ScriptInterruptAwait(ScriptInterrupt reason);
void ScriptResume();
SCRIPTLINETYPE ScriptGetLineTypeLocked(int line);
bool ScriptGetBranchInfoLocked(int line, SCRIPTBRANCH* info);
void ScriptLogLocked(const char* msg);
bool ScriptExecAwait(const char* filename, bool gui);

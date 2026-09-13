#pragma once

#include "debugger.h"

// Arm a process-wide run to the next instruction in party. The caller resumes
// the process (or returns from a debug event); this function never pauses it.
// A setup failure rolls back newly installed breakpoints so the caller can
// single-step. A busy operation is left untouched and also returns false.
bool RunToParty(int party, TITANCBSTEP callback, STEPFUNCTION fallback = StepIntoWow64);
bool RunToPartyIsActive();
void RunToPartyClear();
// A module change invalidates the snapshot. Continue with the saved step mode.
void RunToPartyOnModuleChange();

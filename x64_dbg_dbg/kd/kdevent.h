#pragma once

void KdEventInitialized();
void KdEventUninitialized();
bool KdEventBreakpoint(PDEBUG_BREAKPOINT Breakpoint);
bool KdEventException(PEXCEPTION_RECORD64 Exception, ULONG FirstChance);
bool KdEventResumed();
bool KdEventPaused();
bool KdEventSingleStepOver();
bool KdEventSingleStepInto();
bool KdEventSingleStepBranch();
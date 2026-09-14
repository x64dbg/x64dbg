#pragma once

#include "_global.h"
#include "TitanEngine/TitanEngine.h"

// Wrapper around the debug engine's SetBPX that gives plugins a say in setting
// user breakpoints.
//
// CB_BEFORE_SETBPX fires while the target address still holds its original
// bytes, before anything is written to the debuggee. A plugin may cancel there
// to stop the breakpoint from being set at all. CB_AFTER_SETBPX fires once the
// engine is done, successful or not.
//
// Only the paths that set *user* breakpoints call this. Internal breakpoints
// (pause, LoadLibrary/FreeLibrary stubs, exception dispatch) keep calling
// SetBPX directly and stay invisible to plugins.
//
// Must not be called while holding LockBreakpoints: the plugin callbacks are
// free to call back into the x64dbg APIs.
bool SetBPXHooked(ULONG_PTR bpxAddress, DWORD bpxType, TITANCBSOFTBP bpxCallBack);

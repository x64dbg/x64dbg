#pragma once

#include "command.h"

bool cbDebugSetBPX(int argc, char* argv[]);
bool cbDebugDeleteBPX(int argc, char* argv[]);
bool cbDebugEnableBPX(int argc, char* argv[]);
bool cbDebugDisableBPX(int argc, char* argv[]);
bool cbDebugSetHardwareBreakpoint(int argc, char* argv[]);
bool cbDebugDeleteHardwareBreakpoint(int argc, char* argv[]);
bool cbDebugEnableHardwareBreakpoint(int argc, char* argv[]);
bool cbDebugDisableHardwareBreakpoint(int argc, char* argv[]);
bool cbDebugSetMemoryBpx(int argc, char* argv[]);
bool cbDebugDeleteMemoryBreakpoint(int argc, char* argv[]);
bool cbDebugEnableMemoryBreakpoint(int argc, char* argv[]);
bool cbDebugDisableMemoryBreakpoint(int argc, char* argv[]);
bool cbDebugBpDll(int argc, char* argv[]);
bool cbDebugBcDll(int argc, char* argv[]);
bool cbDebugBpDllEnable(int argc, char* argv[]);
bool cbDebugBpDllDisable(int argc, char* argv[]);
bool cbDebugSetExceptionBPX(int argc, char* argv[]);
bool cbDebugDeleteExceptionBPX(int argc, char* argv[]);
bool cbDebugEnableExceptionBPX(int argc, char* argv[]);
bool cbDebugDisableExceptionBPX(int argc, char* argv[]);
bool cbDebugSetBPGoto(int argc, char* argv[]);
bool cbDebugBplist(int argc, char* argv[]);
bool cbDebugSetBPXOptions(int argc, char* argv[]);

#pragma once

#include "command.h"

CMDRESULT cbDebugSetBPX(int argc, char* argv[]);
CMDRESULT cbDebugDeleteBPX(int argc, char* argv[]);
CMDRESULT cbDebugEnableBPX(int argc, char* argv[]);
CMDRESULT cbDebugDisableBPX(int argc, char* argv[]);
CMDRESULT cbDebugSetHardwareBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugDeleteHardwareBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugEnableHardwareBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugDisableHardwareBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugSetMemoryBpx(int argc, char* argv[]);
CMDRESULT cbDebugDeleteMemoryBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugEnableMemoryBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugDisableMemoryBreakpoint(int argc, char* argv[]);
CMDRESULT cbDebugBpDll(int argc, char* argv[]);
CMDRESULT cbDebugBcDll(int argc, char* argv[]);
CMDRESULT cbDebugBpDllEnable(int argc, char* argv[]);
CMDRESULT cbDebugBpDllDisable(int argc, char* argv[]);
CMDRESULT cbDebugSetExceptionBPX(int argc, char* argv[]);
CMDRESULT cbDebugDeleteExceptionBPX(int argc, char* argv[]);
CMDRESULT cbDebugEnableExceptionBPX(int argc, char* argv[]);
CMDRESULT cbDebugDisableExceptionBPX(int argc, char* argv[]);
CMDRESULT cbDebugSetBPGoto(int argc, char* argv[]);
CMDRESULT cbDebugBplist(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXOptions(int argc, char* argv[]);

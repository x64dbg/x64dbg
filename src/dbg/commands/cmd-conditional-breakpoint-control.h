#pragma once

#include "command.h"

bool cbDebugSetBPXName(int argc, char* argv[]);
bool cbDebugSetBPXCondition(int argc, char* argv[]);
bool cbDebugSetBPXLog(int argc, char* argv[]);
bool cbDebugSetBPXLogCondition(int argc, char* argv[]);
bool cbDebugSetBPXCommand(int argc, char* argv[]);
bool cbDebugSetBPXCommandCondition(int argc, char* argv[]);
bool cbDebugSetBPXLogFile(int argc, char* argv[]);
bool cbDebugSetBPXFastResume(int argc, char* argv[]);
bool cbDebugSetBPXSingleshoot(int argc, char* argv[]);
bool cbDebugSetBPXSilent(int argc, char* argv[]);
bool cbDebugGetBPXHitCount(int argc, char* argv[]);
bool cbDebugResetBPXHitCount(int argc, char* argv[]);

bool cbDebugSetBPXHardwareName(int argc, char* argv[]);
bool cbDebugSetBPXHardwareCondition(int argc, char* argv[]);
bool cbDebugSetBPXHardwareLog(int argc, char* argv[]);
bool cbDebugSetBPXHardwareLogCondition(int argc, char* argv[]);
bool cbDebugSetBPXHardwareCommand(int argc, char* argv[]);
bool cbDebugSetBPXHardwareCommandCondition(int argc, char* argv[]);
bool cbDebugSetBPXHardwareLogFile(int argc, char* argv[]);
bool cbDebugSetBPXHardwareFastResume(int argc, char* argv[]);
bool cbDebugSetBPXHardwareSingleshoot(int argc, char* argv[]);
bool cbDebugSetBPXHardwareSilent(int argc, char* argv[]);
bool cbDebugGetBPXHardwareHitCount(int argc, char* argv[]);
bool cbDebugResetBPXHardwareHitCount(int argc, char* argv[]);

bool cbDebugSetBPXMemoryName(int argc, char* argv[]);
bool cbDebugSetBPXMemoryCondition(int argc, char* argv[]);
bool cbDebugSetBPXMemoryLog(int argc, char* argv[]);
bool cbDebugSetBPXMemoryLogCondition(int argc, char* argv[]);
bool cbDebugSetBPXMemoryCommand(int argc, char* argv[]);
bool cbDebugSetBPXMemoryCommandCondition(int argc, char* argv[]);
bool cbDebugSetBPXMemoryLogFile(int argc, char* argv[]);
bool cbDebugSetBPXMemoryFastResume(int argc, char* argv[]);
bool cbDebugSetBPXMemorySingleshoot(int argc, char* argv[]);
bool cbDebugSetBPXMemorySilent(int argc, char* argv[]);
bool cbDebugGetBPXMemoryHitCount(int argc, char* argv[]);
bool cbDebugResetBPXMemoryHitCount(int argc, char* argv[]);

bool cbDebugSetBPXDLLName(int argc, char* argv[]);
bool cbDebugSetBPXDLLCondition(int argc, char* argv[]);
bool cbDebugSetBPXDLLLog(int argc, char* argv[]);
bool cbDebugSetBPXDLLLogCondition(int argc, char* argv[]);
bool cbDebugSetBPXDLLCommand(int argc, char* argv[]);
bool cbDebugSetBPXDLLCommandCondition(int argc, char* argv[]);
bool cbDebugSetBPXDLLLogFile(int argc, char* argv[]);
bool cbDebugSetBPXDLLFastResume(int argc, char* argv[]);
bool cbDebugSetBPXDLLSingleshoot(int argc, char* argv[]);
bool cbDebugSetBPXDLLSilent(int argc, char* argv[]);
bool cbDebugGetBPXDLLHitCount(int argc, char* argv[]);
bool cbDebugResetBPXDLLHitCount(int argc, char* argv[]);

bool cbDebugSetBPXExceptionName(int argc, char* argv[]);
bool cbDebugSetBPXExceptionCondition(int argc, char* argv[]);
bool cbDebugSetBPXExceptionLog(int argc, char* argv[]);
bool cbDebugSetBPXExceptionLogCondition(int argc, char* argv[]);
bool cbDebugSetBPXExceptionCommand(int argc, char* argv[]);
bool cbDebugSetBPXExceptionCommandCondition(int argc, char* argv[]);
bool cbDebugSetBPXExceptionLogFile(int argc, char* argv[]);
bool cbDebugSetBPXExceptionFastResume(int argc, char* argv[]);
bool cbDebugSetBPXExceptionSingleshoot(int argc, char* argv[]);
bool cbDebugSetBPXExceptionSilent(int argc, char* argv[]);
bool cbDebugGetBPXExceptionHitCount(int argc, char* argv[]);
bool cbDebugResetBPXExceptionHitCount(int argc, char* argv[]);
#pragma once

#include "command.h"

CMDRESULT cbDebugSetBPXName(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXLog(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXLogCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXCommand(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXCommandCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXFastResume(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXSingleshoot(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXSilent(int argc, char* argv[]);
CMDRESULT cbDebugGetBPXHitCount(int argc, char* argv[]);
CMDRESULT cbDebugResetBPXHitCount(int argc, char* argv[]);

CMDRESULT cbDebugSetBPXHardwareName(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXHardwareCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXHardwareLog(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXHardwareLogCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXHardwareCommand(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXHardwareCommandCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXHardwareFastResume(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXHardwareSingleshoot(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXHardwareSilent(int argc, char* argv[]);
CMDRESULT cbDebugGetBPXHardwareHitCount(int argc, char* argv[]);
CMDRESULT cbDebugResetBPXHardwareHitCount(int argc, char* argv[]);

CMDRESULT cbDebugSetBPXMemoryName(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXMemoryCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXMemoryLog(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXMemoryLogCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXMemoryCommand(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXMemoryCommandCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXMemoryFastResume(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXMemorySingleshoot(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXMemorySilent(int argc, char* argv[]);
CMDRESULT cbDebugGetBPXMemoryHitCount(int argc, char* argv[]);
CMDRESULT cbDebugResetBPXMemoryHitCount(int argc, char* argv[]);

CMDRESULT cbDebugSetBPXDLLName(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXDLLCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXDLLLog(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXDLLLogCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXDLLCommand(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXDLLCommandCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXDLLFastResume(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXDLLSingleshoot(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXDLLSilent(int argc, char* argv[]);
CMDRESULT cbDebugGetBPXDLLHitCount(int argc, char* argv[]);
CMDRESULT cbDebugResetBPXDLLHitCount(int argc, char* argv[]);

CMDRESULT cbDebugSetBPXExceptionName(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXExceptionCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXExceptionLog(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXExceptionLogCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXExceptionCommand(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXExceptionCommandCondition(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXExceptionFastResume(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXExceptionSingleshoot(int argc, char* argv[]);
CMDRESULT cbDebugSetBPXExceptionSilent(int argc, char* argv[]);
CMDRESULT cbDebugGetBPXExceptionHitCount(int argc, char* argv[]);
CMDRESULT cbDebugResetBPXExceptionHitCount(int argc, char* argv[]);
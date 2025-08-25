#pragma once

#include "command.h"

bool cbDebugTraceIntoConditional(int argc, char* argv[]);
bool cbDebugTraceOverConditional(int argc, char* argv[]);
bool cbDebugTraceIntoBeyondTraceRecord(int argc, char* argv[]);
bool cbDebugTraceOverBeyondTraceRecord(int argc, char* argv[]);
bool cbDebugTraceIntoIntoTraceRecord(int argc, char* argv[]);
bool cbDebugTraceOverIntoTraceRecord(int argc, char* argv[]);
bool cbDebugTraceWithModules(int argc, char* argv[]);
bool cbDebugRunToParty(int argc, char* argv[]);
bool cbDebugRunToUserCode(int argc, char* argv[]);
bool cbDebugTraceSetLog(int argc, char* argv[]);
bool cbDebugTraceSetCommand(int argc, char* argv[]);
bool cbDebugTraceSetLogFile(int argc, char* argv[]);
bool cbDebugStartTraceRecording(int argc, char* argv[]);
bool cbDebugStopTraceRecording(int argc, char* argv[]);
bool cbAddTraceExcludedModule(int argc, char* argv[]);
bool cbDelTraceExcludedModule(int argc, char* argv[]);
bool cbListTraceExcludedModules(int argc, char* argv[]);
bool cbClearTraceExcludedModules(int argc, char* argv[]);
bool cbAddTraceIncludedModule(int argc, char* argv[]);
bool cbDelTraceIncludedModule(int argc, char* argv[]);
bool cbListTraceIncludedModules(int argc, char* argv[]);
bool cbClearTraceIncludedModules(int argc, char* argv[]);
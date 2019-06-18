#pragma once

#include "command.h"

bool cbInstrAnalyse(int argc, char* argv[]);
bool cbInstrExanalyse(int argc, char* argv[]);
bool cbInstrCfanalyse(int argc, char* argv[]);
bool cbInstrAnalyseNukem(int argc, char* argv[]);
bool cbInstrAnalxrefs(int argc, char* argv[]);
bool cbInstrAnalrecur(int argc, char* argv[]);
bool cbInstrAnalyseadv(int argc, char* argv[]);

bool cbInstrVirtualmod(int argc, char* argv[]);
bool cbDebugDownloadSymbol(int argc, char* argv[]);
bool cbDebugLoadSymbol(int argc, char* argv[]);
bool cbDebugUnloadSymbol(int argc, char* argv[]);
bool cbInstrImageinfo(int argc, char* argv[]);
bool cbInstrGetRelocSize(int argc, char* argv[]);
bool cbInstrExhandlers(int argc, char* argv[]);
bool cbInstrExinfo(int argc, char* argv[]);
bool cbInstrTraceexecute(int argc, char* argv[]);
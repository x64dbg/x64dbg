#pragma once

#include "command.h"

CMDRESULT cbInstrAnalyse(int argc, char* argv[]);
CMDRESULT cbInstrExanalyse(int argc, char* argv[]);
CMDRESULT cbInstrCfanalyse(int argc, char* argv[]);
CMDRESULT cbInstrAnalyseNukem(int argc, char* argv[]);
CMDRESULT cbInstrAnalxrefs(int argc, char* argv[]);
CMDRESULT cbInstrAnalrecur(int argc, char* argv[]);
CMDRESULT cbInstrAnalyseadv(int argc, char* argv[]);

CMDRESULT cbInstrVirtualmod(int argc, char* argv[]);
CMDRESULT cbDebugDownloadSymbol(int argc, char* argv[]);
CMDRESULT cbInstrImageinfo(int argc, char* argv[]);
CMDRESULT cbInstrGetRelocSize(int argc, char* argv[]);
CMDRESULT cbInstrExhandlers(int argc, char* argv[]);
CMDRESULT cbInstrExinfo(int argc, char* argv[]);
CMDRESULT cbInstrTraceexecute(int argc, char* argv[]);
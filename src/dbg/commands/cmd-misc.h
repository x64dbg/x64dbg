#pragma once

#include "command.h"

CMDRESULT cbInstrChd(int argc, char* argv[]);
CMDRESULT cbInstrZzz(int argc, char* argv[]);

CMDRESULT cbDebugHide(int argc, char* argv[]);
CMDRESULT cbDebugLoadLib(int argc, char* argv[]);
CMDRESULT cbInstrAssemble(int argc, char* argv[]);
CMDRESULT cbInstrGpa(int argc, char* argv[]);

CMDRESULT cbDebugSetJIT(int argc, char* argv[]);
CMDRESULT cbDebugGetJIT(int argc, char* argv[]);
CMDRESULT cbDebugGetJITAuto(int argc, char* argv[]);
CMDRESULT cbDebugSetJITAuto(int argc, char* argv[]);

CMDRESULT cbDebugGetCmdline(int argc, char* argv[]);
CMDRESULT cbDebugSetCmdline(int argc, char* argv[]);

CMDRESULT cbInstrMnemonichelp(int argc, char* argv[]);
CMDRESULT cbInstrMnemonicbrief(int argc, char* argv[]);
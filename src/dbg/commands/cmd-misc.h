#pragma once

#include "command.h"

bool cbInstrChd(int argc, char* argv[]);
bool cbInstrZzz(int argc, char* argv[]);

bool cbDebugHide(int argc, char* argv[]);
bool cbDebugLoadLib(int argc, char* argv[]);
bool cbDebugFreeLib(int argc, char* argv[]);
bool cbInstrAssemble(int argc, char* argv[]);
bool cbInstrGpa(int argc, char* argv[]);

bool cbDebugSetJIT(int argc, char* argv[]);
bool cbDebugGetJIT(int argc, char* argv[]);
bool cbDebugGetJITAuto(int argc, char* argv[]);
bool cbDebugSetJITAuto(int argc, char* argv[]);

bool cbDebugGetCmdline(int argc, char* argv[]);
bool cbDebugSetCmdline(int argc, char* argv[]);

bool cbInstrMnemonichelp(int argc, char* argv[]);
bool cbInstrMnemonicbrief(int argc, char* argv[]);

bool cbInstrConfig(int argc, char* argv[]);
bool cbInstrRestartadmin(int argc, char* argv[]);
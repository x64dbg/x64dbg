#pragma once

#include "command.h"

CMDRESULT cbInstrInc(int argc, char* argv[]);
CMDRESULT cbInstrDec(int argc, char* argv[]);
CMDRESULT cbInstrAdd(int argc, char* argv[]);
CMDRESULT cbInstrSub(int argc, char* argv[]);
CMDRESULT cbInstrMul(int argc, char* argv[]);
CMDRESULT cbInstrDiv(int argc, char* argv[]);
CMDRESULT cbInstrAnd(int argc, char* argv[]);
CMDRESULT cbInstrOr(int argc, char* argv[]);
CMDRESULT cbInstrXor(int argc, char* argv[]);
CMDRESULT cbInstrNeg(int argc, char* argv[]);
CMDRESULT cbInstrNot(int argc, char* argv[]);
CMDRESULT cbInstrBswap(int argc, char* argv[]);
CMDRESULT cbInstrRol(int argc, char* argv[]);
CMDRESULT cbInstrRor(int argc, char* argv[]);
CMDRESULT cbInstrShl(int argc, char* argv[]);
CMDRESULT cbInstrShr(int argc, char* argv[]);
CMDRESULT cbInstrSar(int argc, char* argv[]);
CMDRESULT cbInstrPush(int argc, char* argv[]);
CMDRESULT cbInstrPop(int argc, char* argv[]);
CMDRESULT cbInstrTest(int argc, char* argv[]);
CMDRESULT cbInstrCmp(int argc, char* argv[]);
CMDRESULT cbInstrMov(int argc, char* argv[]);
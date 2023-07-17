#pragma once

#include "command.h"

bool cbInstrInc(int argc, char* argv[]);
bool cbInstrDec(int argc, char* argv[]);
bool cbInstrAdd(int argc, char* argv[]);
bool cbInstrSub(int argc, char* argv[]);
bool cbInstrMul(int argc, char* argv[]);
bool cbInstrDiv(int argc, char* argv[]);
bool cbInstrAnd(int argc, char* argv[]);
bool cbInstrOr(int argc, char* argv[]);
bool cbInstrXor(int argc, char* argv[]);
bool cbInstrNeg(int argc, char* argv[]);
bool cbInstrNot(int argc, char* argv[]);
bool cbInstrBswap(int argc, char* argv[]);
bool cbInstrRol(int argc, char* argv[]);
bool cbInstrRor(int argc, char* argv[]);
bool cbInstrShl(int argc, char* argv[]);
bool cbInstrShr(int argc, char* argv[]);
bool cbInstrSar(int argc, char* argv[]);
bool cbInstrPush(int argc, char* argv[]);
bool cbInstrPop(int argc, char* argv[]);
bool cbInstrTest(int argc, char* argv[]);
bool cbInstrCmp(int argc, char* argv[]);
bool cbInstrMov(int argc, char* argv[]);

bool cbInstrMovdqu(int argc, char* argv[]);
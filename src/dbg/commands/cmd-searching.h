#pragma once

#include "command.h"

bool cbInstrFind(int argc, char* argv[]);
bool cbInstrFindAll(int argc, char* argv[]);
bool cbInstrFindAllMem(int argc, char* argv[]);
bool cbInstrFindAsm(int argc, char* argv[]);
bool cbInstrRefFind(int argc, char* argv[]);
bool cbInstrRefFindRange(int argc, char* argv[]);
bool cbInstrRefStr(int argc, char* argv[]);
bool cbInstrRefFuncionPointer(int argc, char* argv[]);
bool cbInstrModCallFind(int argc, char* argv[]);
bool cbInstrGUIDFind(int argc, char* argv[]);
bool cbInstrYara(int argc, char* argv[]);
bool cbInstrYaramod(int argc, char* argv[]);
bool cbInstrSetMaxFindResult(int argc, char* argv[]);

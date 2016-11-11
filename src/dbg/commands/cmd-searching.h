#pragma once

#include "command.h"

CMDRESULT cbInstrFind(int argc, char* argv[]);
CMDRESULT cbInstrFindAll(int argc, char* argv[]);
CMDRESULT cbInstrFindAllMem(int argc, char* argv[]);
CMDRESULT cbInstrFindAsm(int argc, char* argv[]);
CMDRESULT cbInstrRefFind(int argc, char* argv[]);
CMDRESULT cbInstrRefFindRange(int argc, char* argv[]);
CMDRESULT cbInstrRefStr(int argc, char* argv[]);
CMDRESULT cbInstrModCallFind(int argc, char* argv[]);
CMDRESULT cbInstrGUIDFind(int argc, char* argv[]);
CMDRESULT cbInstrYara(int argc, char* argv[]);
CMDRESULT cbInstrYaramod(int argc, char* argv[]);
CMDRESULT cbInstrSetMaxFindResult(int argc, char* argv[]);

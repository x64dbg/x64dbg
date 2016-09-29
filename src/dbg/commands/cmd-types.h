#pragma once

#include "command.h"

CMDRESULT cbInstrDataUnknown(int argc, char* argv[]);
CMDRESULT cbInstrDataByte(int argc, char* argv[]);
CMDRESULT cbInstrDataWord(int argc, char* argv[]);
CMDRESULT cbInstrDataDword(int argc, char* argv[]);
CMDRESULT cbInstrDataFword(int argc, char* argv[]);
CMDRESULT cbInstrDataQword(int argc, char* argv[]);
CMDRESULT cbInstrDataTbyte(int argc, char* argv[]);
CMDRESULT cbInstrDataOword(int argc, char* argv[]);
CMDRESULT cbInstrDataMmword(int argc, char* argv[]);
CMDRESULT cbInstrDataXmmword(int argc, char* argv[]);
CMDRESULT cbInstrDataYmmword(int argc, char* argv[]);
CMDRESULT cbInstrDataFloat(int argc, char* argv[]);
CMDRESULT cbInstrDataDouble(int argc, char* argv[]);
CMDRESULT cbInstrDataLongdouble(int argc, char* argv[]);
CMDRESULT cbInstrDataAscii(int argc, char* argv[]);
CMDRESULT cbInstrDataUnicode(int argc, char* argv[]);
CMDRESULT cbInstrDataCode(int argc, char* argv[]);
CMDRESULT cbInstrDataJunk(int argc, char* argv[]);
CMDRESULT cbInstrDataMiddle(int argc, char* argv[]);
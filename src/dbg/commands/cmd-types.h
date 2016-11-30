#pragma once

#include "command.h"

bool cbInstrDataUnknown(int argc, char* argv[]);
bool cbInstrDataByte(int argc, char* argv[]);
bool cbInstrDataWord(int argc, char* argv[]);
bool cbInstrDataDword(int argc, char* argv[]);
bool cbInstrDataFword(int argc, char* argv[]);
bool cbInstrDataQword(int argc, char* argv[]);
bool cbInstrDataTbyte(int argc, char* argv[]);
bool cbInstrDataOword(int argc, char* argv[]);
bool cbInstrDataMmword(int argc, char* argv[]);
bool cbInstrDataXmmword(int argc, char* argv[]);
bool cbInstrDataYmmword(int argc, char* argv[]);
bool cbInstrDataFloat(int argc, char* argv[]);
bool cbInstrDataDouble(int argc, char* argv[]);
bool cbInstrDataLongdouble(int argc, char* argv[]);
bool cbInstrDataAscii(int argc, char* argv[]);
bool cbInstrDataUnicode(int argc, char* argv[]);
bool cbInstrDataCode(int argc, char* argv[]);
bool cbInstrDataJunk(int argc, char* argv[]);
bool cbInstrDataMiddle(int argc, char* argv[]);

bool cbInstrAddType(int argc, char* argv[]);
bool cbInstrAddStruct(int argc, char* argv[]);
bool cbInstrAddUnion(int argc, char* argv[]);
bool cbInstrAddMember(int argc, char* argv[]);
bool cbInstrAppendMember(int argc, char* argv[]);
bool cbInstrAddFunction(int argc, char* argv[]);
bool cbInstrAddArg(int argc, char* argv[]);
bool cbInstrAppendArg(int argc, char* argv[]);
bool cbInstrSizeofType(int argc, char* argv[]);
bool cbInstrVisitType(int argc, char* argv[]);
bool cbInstrClearTypes(int argc, char* argv[]);
bool cbInstrRemoveType(int argc, char* argv[]);
bool cbInstrEnumTypes(int argc, char* argv[]);
bool cbInstrLoadTypes(int argc, char* argv[]);
bool cbInstrParseTypes(int argc, char* argv[]);
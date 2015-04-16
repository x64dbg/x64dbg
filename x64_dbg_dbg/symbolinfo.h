#pragma once

#include "_global.h"

void SymEnum(uint Base, CBSYMBOLENUM EnumCallback, void* UserData);
bool SymGetModuleList(std::vector<SYMBOLMODULEINFO>* List);
void SymUpdateModuleList();
void SymDownloadAllSymbols(const char* SymbolStore);
bool SymAddrFromName(const char* Name, uint* Address);
const char* SymGetSymbolicName(uint Address);

/**
\brief Gets the source code file name and line from an address.
\param cip The address to check.
\param [out] szFileName Source code file. Buffer of MAX_STRING_SIZE length. UTF-8. Can be null.
\param [out] nLine Line number. Can be null.
\return true if it succeeds, false if it fails.
*/
bool SymGetSourceLine(uint Cip, char* FileName, int* Line);
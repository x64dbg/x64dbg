#pragma once

#include "_global.h"

void SymEnum(duint Base, CBSYMBOLENUM EnumCallback, void* UserData, bool bUseCache);
bool SymGetModuleList(std::vector<SYMBOLMODULEINFO>* List);
void SymUpdateModuleList();
void SymDownloadAllSymbols(const char* SymbolStore);
bool SymAddrFromName(const char* Name, duint* Address);
const char* SymGetSymbolicName(duint Address);
void SymClearMemoryCache();

/**
\brief Gets the source code file name and line from an address.
\param cip The address to check.
\param [out] szFileName Source code file. Buffer of MAX_STRING_SIZE length. UTF-8. Can be null.
\param [out] nLine Line number. Can be null.
\return true if it succeeds, false if it fails.
*/
bool SymGetSourceLine(duint Cip, char* FileName, int* Line);
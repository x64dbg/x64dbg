#pragma once

#include "_global.h"

void SymEnum(uint Base, CBSYMBOLENUM EnumCallback, void* UserData);
bool SymGetModuleList(std::vector<SYMBOLMODULEINFO>* List);
void SymUpdateModuleList();
void SymDownloadAllSymbols(const char* SymbolStore);
bool SymAddrFromName(const char* Name, uint* Address);
const char* SymGetSymbolicName(uint Address);
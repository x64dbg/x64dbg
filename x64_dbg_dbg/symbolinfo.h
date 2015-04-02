#ifndef _SYMBOLINFO_H
#define _SYMBOLINFO_H

#include "_global.h"

void symenum(uint base, CBSYMBOLENUM cbSymbolEnum, void* user);
void symupdatemodulelist();
void symdownloadallsymbols(const char* szSymbolStore);
bool symfromname(const char* name, uint* addr);
const char* symgetsymbolicname(uint addr);

#endif //_SYMBOLINFO_H

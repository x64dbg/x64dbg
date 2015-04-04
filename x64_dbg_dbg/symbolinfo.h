#ifndef _SYMBOLINFO_H
#define _SYMBOLINFO_H

#include "_global.h"

void symenum(uint base, CBSYMBOLENUM cbSymbolEnum, void* user);
void symupdatemodulelist();
void symdownloadallsymbols(const char* szSymbolStore);
bool symfromname(const char* name, uint* addr);
const char* symgetsymbolicname(uint addr);

/**
\brief Gets the source code file name and line from an address.
\param cip The address to check.
\param [out] szFileName Source code file. Buffer of MAX_STRING_SIZE length. UTF-8. Can be null.
\param [out] nLine Line number. Can be null.
\return true if it succeeds, false if it fails.
*/
bool symgetsourceline(uint cip, char* szFileName, int* nLine);

#endif //_SYMBOLINFO_H

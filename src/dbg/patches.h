#ifndef _PATCHES_H
#define _PATCHES_H

#include "_global.h"

//casted to bridgemain.h:DBGPATCHINFO in _dbgfunctions.cpp
struct PATCHINFO
{
    char mod[MAX_MODULE_SIZE];
    duint addr;
    unsigned char oldbyte;
    unsigned char newbyte;
};

bool PatchSet(duint Address, unsigned char OldByte, unsigned char NewByte);
bool PatchGet(duint Address, PATCHINFO* Patch);
bool PatchDelete(duint Address, bool Restore);
void PatchDelRange(duint Start, duint End, bool Restore);
bool PatchEnum(PATCHINFO* List, size_t* Size);
int PatchFile(const PATCHINFO* List, int Count, const char* FileName, char* Error);
void PatchClear(const char* Module = nullptr);

#endif // _PATCHES_H
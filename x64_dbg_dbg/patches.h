#pragma once

#include "_global.h"

struct PATCHINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    unsigned char oldbyte;
    unsigned char newbyte;
};

bool PatchSet(uint Address, unsigned char OldByte, unsigned char NewByte);
bool PatchGet(uint Address, PATCHINFO* Patch);
bool PatchDelete(uint Address, bool Restore);
void PatchDelRange(uint Start, uint End, bool Restore);
bool PatchEnum(PATCHINFO* List, size_t* Size);
int PatchFile(const PATCHINFO* List, int Count, const char* FileName, char* Error);
void PatchClear(const char* Module = nullptr);
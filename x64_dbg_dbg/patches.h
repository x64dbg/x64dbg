#ifndef _PATCHES_H
#define _PATCHES_H

#include "_global.h"

struct PATCHINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    unsigned char oldbyte;
    unsigned char newbyte;
};
typedef std::map<uint, PATCHINFO> PatchesInfo;

bool patchset(uint addr, unsigned char oldbyte, unsigned char newbyte);
bool patchget(uint addr, PATCHINFO* patch);
bool patchdel(uint addr, bool restore);
void patchdelrange(uint start, uint end, bool restore);
void patchclear(const char* mod = 0);
bool patchenum(PATCHINFO* patchlist, size_t* cbsize);
int patchfile(const PATCHINFO* patchlist, int count, const char* szFileName, char* error);

#endif //_PATCHES_H

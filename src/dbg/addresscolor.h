#ifndef _ADDRESSCOLOR_H
#define _ADDRESSCOLOR_H

#include "_global.h"
#include "addrinfo.h"

struct ADDRESSCOLORINFO : RangeInfo
{
    duint color;
};

bool AddressColorSet(duint Address, duint color, bool Manual);
bool AddressColorSetRange(duint Start, duint End, duint color, bool Manual);
bool AddressColorGet(duint Address, duint* color);
bool AddressColorDelete(duint Address);
void AddressColorDelRange(duint Start, duint End, bool Manual);
void AddressColorCacheSave(JSON Root);
void AddressColorCacheLoad(JSON Root);
bool AddressColorEnum(ADDRESSCOLORINFO* List, size_t* Size);
void AddressColorClear(bool Terminating);
void AddressColorGetList(std::vector<ADDRESSCOLORINFO> & list);
bool AddressColorGetInfo(duint Address, ADDRESSCOLORINFO* info);

#endif // _ADDRESSCOLOR_H
#ifndef _ARGUMENT_H
#define _ARGUMENT_H

#include "addrinfo.h"

struct ARGUMENTSINFO
{
    duint modhash;
    duint start;
    duint end;
    bool manual;
    duint instructioncount;

    std::string mod() const
    {
        return ModNameFromHash(modhash);
    }
};

bool ArgumentAdd(duint Start, duint End, bool Manual, duint InstructionCount = 0);
bool ArgumentGet(duint Address, duint* Start = nullptr, duint* End = nullptr, duint* InstrCount = nullptr);
bool ArgumentOverlaps(duint Start, duint End);
bool ArgumentDelete(duint Address);
void ArgumentDelRange(duint Start, duint End, bool DeleteManual = false);
void ArgumentCacheSave(rapidjson::Document & Root);
void ArgumentCacheLoad(rapidjson::Document & Root);
void ArgumentClear();
void ArgumentGetList(std::vector<ARGUMENTSINFO> & list);
bool ArgumentGetInfo(duint Address, ARGUMENTSINFO & info);
bool ArgumentEnum(ARGUMENTSINFO* List, size_t* Size);

#endif // _ARGUMENT_H
#pragma once

#include "addrinfo.h"

struct FUNCTIONSINFO
{
    char mod[MAX_MODULE_SIZE];
    duint start;
    duint end;
    bool manual;
};

bool FunctionAdd(duint Start, duint End, bool Manual);
bool FunctionGet(duint Address, duint* Start, duint* End);
bool FunctionOverlaps(duint Start, duint End);
bool FunctionDelete(duint Address);
void FunctionDelRange(duint Start, duint End);
void FunctionCacheSave(JSON Root);
void FunctionCacheLoad(JSON Root);
bool FunctionEnum(FUNCTIONSINFO* List, size_t* Size);
void FunctionClear();
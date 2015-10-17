#pragma once

#include "addrinfo.h"

struct FUNCTIONSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint start;
    uint end;
    bool manual;
};

bool FunctionAdd(uint Start, uint End, bool Manual);
bool FunctionGet(uint Address, uint* Start, uint* End);
bool FunctionOverlaps(uint Start, uint End);
bool FunctionDelete(uint Address);
void FunctionDelRange(uint Start, uint End);
void FunctionCacheSave(JSON Root);
void FunctionCacheLoad(JSON Root);
bool FunctionEnum(FUNCTIONSINFO* List, size_t* Size);
void FunctionClear();
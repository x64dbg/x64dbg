#ifndef _LOOP_H
#define _LOOP_H

#include "addrinfo.h"

struct LOOPSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint start;
    uint end;
    uint parent;
    int depth;
    bool manual;
};

bool LoopAdd(uint Start, uint End, bool Manual);
bool LoopGet(int Depth, uint Address, uint* Start, uint* End);
bool LoopOverlaps(int Depth, uint Start, uint End, int* FinalDepth);
bool LoopDelete(int Depth, uint Address);
void LoopCacheSave(JSON Root);
void LoopCacheLoad(JSON Root);
bool LoopEnum(LOOPSINFO* List, size_t* Size);
void LoopClear();

#endif //_LOOP_H
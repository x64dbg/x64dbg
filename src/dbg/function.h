#ifndef _FUNCTION_H
#define _FUNCTION_H

#include "addrinfo.h"

struct FUNCTIONSINFO : RangeInfo
{
    duint instructioncount;
    duint parent;
};

bool FunctionAdd(duint Start, duint End, bool Manual, duint InstructionCount = 0, duint Parent = 0);
bool FunctionGet(duint Address, duint* Start = nullptr, duint* End = nullptr, duint* InstrCount = nullptr, duint* Parent = nullptr);
bool FunctionOverlaps(duint Start, duint End);
bool FunctionDelete(duint Address);
void FunctionDelRange(duint Start, duint End, bool DeleteManual = false);
void FunctionCacheSave(JSON Root);
void FunctionCacheLoad(JSON Root);
bool FunctionEnum(FUNCTIONSINFO* List, size_t* Size);
void FunctionClear(bool Terminating);
void FunctionGetList(std::vector<FUNCTIONSINFO> & list);
bool FunctionGetInfo(duint Address, FUNCTIONSINFO & info);

#endif // _FUNCTION_H
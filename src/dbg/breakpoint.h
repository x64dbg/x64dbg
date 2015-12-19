#pragma once

#include "_global.h"

#define TITANSETDRX(titantype, drx) titantype &= 0x0FF; titantype |= (drx<<8)
#define TITANGETDRX(titantype) (titantype >> 8) & 0xF
#define TITANSETTYPE(titantype, type) titantype &= 0xF0F; titantype |= (type<<4)
#define TITANGETTYPE(titantype) (titantype >> 4) & 0xF
#define TITANSETSIZE(titantype, size) titantype &= 0xFF0; titantype |= size;
#define TITANGETSIZE(titantype) titantype & 0xF

enum BP_TYPE
{
    BPNORMAL = 0,
    BPHARDWARE = 1,
    BPMEMORY = 2
};

struct BREAKPOINT
{
    duint addr;
    bool enabled;
    bool singleshoot;
    bool active;
    unsigned short oldbytes;
    BP_TYPE type;
    DWORD titantype;
    char name[MAX_BREAKPOINT_SIZE];
    char mod[MAX_MODULE_SIZE];
};

// Breakpoint enumeration callback
typedef bool (*BPENUMCALLBACK)(const BREAKPOINT* bp);

BREAKPOINT* BpInfoFromAddr(BP_TYPE Type, duint Address);
int BpGetList(std::vector<BREAKPOINT>* List);
bool BpNew(duint Address, bool Enable, bool Singleshot, short OldBytes, BP_TYPE Type, DWORD TitanType, const char* Name);
bool BpGet(duint Address, BP_TYPE Type, const char* Name, BREAKPOINT* Bp);
bool BpDelete(duint Address, BP_TYPE Type);
bool BpEnable(duint Address, BP_TYPE Type, bool Enable);
bool BpSetName(duint Address, BP_TYPE Type, const char* Name);
bool BpSetTitanType(duint Address, BP_TYPE Type, int TitanType);
bool BpEnumAll(BPENUMCALLBACK EnumCallback, const char* Module);
bool BpEnumAll(BPENUMCALLBACK EnumCallback);
int BpGetCount(BP_TYPE Type, bool EnabledOnly = false);
void BpToBridge(const BREAKPOINT* Bp, BRIDGEBP* BridgeBp);
void BpCacheSave(JSON Root);
void BpCacheLoad(JSON Root);
void BpClear();
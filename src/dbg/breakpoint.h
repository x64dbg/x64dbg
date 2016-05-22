#ifndef _BREAKPOINT_H
#define _BREAKPOINT_H

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
    char condition[MAX_CONDITIONAL_EXPR_SIZE]; // condition to stop. If true, debugger halts.
    char logText[MAX_CONDITIONAL_LOG_SIZE];    // text to log.
    char hitcmd[MAX_CONDITIONAL_EXPR_SIZE];    // script command to execute.
    uint32 hitcount;                           // hit counter
    bool fastResume;                           // if true, debugger resumes without any GUI/Script/Plugin interaction.
};

// Breakpoint enumeration callback
typedef bool (*BPENUMCALLBACK)(const BREAKPOINT* bp);

BREAKPOINT* BpInfoFromAddr(BP_TYPE Type, duint Address);
int BpGetList(std::vector<BREAKPOINT>* List);
bool BpNew(duint Address, bool Enable, bool Singleshot, short OldBytes, BP_TYPE Type, DWORD TitanType, const char* Name);
bool BpGet(duint Address, BP_TYPE Type, const char* Name, BREAKPOINT* Bp);
bool BpGetAny(BP_TYPE Type, const char* Name, BREAKPOINT* Bp);
bool BpDelete(duint Address, BP_TYPE Type);
bool BpEnable(duint Address, BP_TYPE Type, bool Enable);
bool BpSetName(duint Address, BP_TYPE Type, const char* Name);
bool BpSetTitanType(duint Address, BP_TYPE Type, int TitanType);
bool BpSetCondition(duint Address, BP_TYPE Type, const char* Condition);
bool BpSetLogText(duint Address, BP_TYPE Type, const char* Log);
bool BpSetHitCommand(duint Address, BP_TYPE Type, const char* Cmd);
bool BpSetFastResume(duint Address, BP_TYPE Type, bool fastResume);
bool BpEnumAll(BPENUMCALLBACK EnumCallback, const char* Module);
bool BpEnumAll(BPENUMCALLBACK EnumCallback);
int BpGetCount(BP_TYPE Type, bool EnabledOnly = false);
uint32 BpGetHitCount(duint Address, BP_TYPE Type);
bool BpResetHitCount(duint Address, BP_TYPE Type, uint32 newHitCount);
void BpToBridge(const BREAKPOINT* Bp, BRIDGEBP* BridgeBp);
void BpCacheSave(JSON Root);
void BpCacheLoad(JSON Root);
void BpClear();

#endif // _BREAKPOINT_H

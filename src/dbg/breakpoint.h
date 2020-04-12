#ifndef _BREAKPOINT_H
#define _BREAKPOINT_H

#include "_global.h"
#include "jansson/jansson_x64dbg.h"

#define TITANSETDRX(titantype, drx) titantype &= 0x0FF, titantype |= (((drx - UE_DR0) & 0xF) << 8)
#define TITANGETDRX(titantype) UE_DR0 + ((titantype >> 8) & 0xF)
#define TITANDRXVALID(titantype) TITANGETDRX(titantype) != UE_DR7
#define TITANSETTYPE(titantype, type) titantype &= 0xF0F, titantype |= ((type & 0xF) << 4)
#define TITANGETTYPE(titantype) (titantype >> 4) & 0xF
#define TITANSETSIZE(titantype, size) titantype &= 0xFF0, titantype |= (size & 0xF);
#define TITANGETSIZE(titantype) titantype & 0xF

enum BP_TYPE
{
    BPNORMAL = 0,
    BPHARDWARE = 1,
    BPMEMORY = 2,
    BPDLL = 3,
    BPEXCEPTION = 4
};

struct BREAKPOINT
{
    duint addr;                                       // address of the breakpoint (rva relative to base of mod)
    bool enabled;                                     // whether the breakpoint is enabled
    bool singleshoot;                                 // whether the breakpoint should be deleted on first hit
    bool active;                                      // whether the breakpoint is active or not
    bool silent;                                      // whether the breakpoint diplays a default message when hit
    unsigned short oldbytes;                          // original bytes (for software breakpoitns)
    BP_TYPE type;                                     // breakpoint type
    DWORD titantype;                                  // type passed to titanengine
    char name[MAX_BREAKPOINT_SIZE];                   // breakpoint name
    char mod[MAX_MODULE_SIZE];                        // module name
    char breakCondition[MAX_CONDITIONAL_EXPR_SIZE];   // condition to stop. If true, debugger halts.
    char logText[MAX_CONDITIONAL_TEXT_SIZE];          // text to log.
    char logCondition[MAX_CONDITIONAL_EXPR_SIZE];     // condition to log
    char commandText[MAX_CONDITIONAL_TEXT_SIZE];      // script command to execute.
    char commandCondition[MAX_CONDITIONAL_EXPR_SIZE]; // condition to execute the command
    uint32 hitcount;                                  // hit counter
    bool fastResume;                                  // if true, debugger resumes without any GUI/Script/Plugin interaction.
    duint memsize;                                    // memory breakpoint size (not implemented)
};

// Breakpoint enumeration callback
typedef bool (*BPENUMCALLBACK)(const BREAKPOINT* bp);

BREAKPOINT* BpInfoFromAddr(BP_TYPE Type, duint Address);
int BpGetList(std::vector<BREAKPOINT>* List);
bool BpNew(duint Address, bool Enable, bool Singleshot, short OldBytes, BP_TYPE Type, DWORD TitanType, const char* Name, duint memsize = 0);
bool BpNewDll(const char* module, bool Enable, bool Singleshot, DWORD TitanType, const char* Name);
bool BpGet(duint Address, BP_TYPE Type, const char* Name, BREAKPOINT* Bp);
bool BpGetAny(BP_TYPE Type, const char* Name, BREAKPOINT* Bp);
bool BpDelete(duint Address, BP_TYPE Type);
bool BpEnable(duint Address, BP_TYPE Type, bool Enable);
bool BpSetName(duint Address, BP_TYPE Type, const char* Name);
bool BpSetTitanType(duint Address, BP_TYPE Type, int TitanType);
bool BpSetBreakCondition(duint Address, BP_TYPE Type, const char* Condition);
bool BpSetLogText(duint Address, BP_TYPE Type, const char* Log);
bool BpSetLogCondition(duint Address, BP_TYPE Type, const char* Condition);
bool BpSetCommandText(duint Address, BP_TYPE Type, const char* Cmd);
bool BpSetCommandCondition(duint Address, BP_TYPE Type, const char* Condition);
bool BpSetFastResume(duint Address, BP_TYPE Type, bool fastResume);
bool BpSetSingleshoot(duint Address, BP_TYPE Type, bool singleshoot);
bool BpEnumAll(BPENUMCALLBACK EnumCallback, const char* Module, duint base = 0);
bool BpSetSilent(duint Address, BP_TYPE Type, bool silent);
duint BpGetDLLBpAddr(const char* fileName);
bool BpEnumAll(BPENUMCALLBACK EnumCallback);
int BpGetCount(BP_TYPE Type, bool EnabledOnly = false);
uint32 BpGetHitCount(duint Address, BP_TYPE Type);
bool BpResetHitCount(duint Address, BP_TYPE Type, uint32 newHitCount);
void BpToBridge(const BREAKPOINT* Bp, BRIDGEBP* BridgeBp);
void BpCacheSave(JSON Root);
void BpCacheLoad(JSON Root);
void BpClear();
bool BpUpdateDllPath(const char* module1, BREAKPOINT** newBpInfo);

#endif // _BREAKPOINT_H

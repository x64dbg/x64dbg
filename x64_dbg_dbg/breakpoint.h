#ifndef _BREAKPOINT_H
#define _BREAKPOINT_H

#include "_global.h"
#include "TitanEngine\TitanEngine.h"

//macros
#define TITANSETDRX(titantype, drx) titantype &= 0x0FF; titantype |= (drx<<8)
#define TITANGETDRX(titantype) (titantype >> 8) & 0xF
#define TITANSETTYPE(titantype, type) titantype &= 0xF0F; titantype |= (type<<4)
#define TITANGETTYPE(titantype) (titantype >> 4) & 0xF
#define TITANSETSIZE(titantype, size) titantype &= 0xFF0; titantype |= size;
#define TITANGETSIZE(titantype) titantype & 0xF

//enums
enum BP_TYPE
{
    BPNORMAL = 0,
    BPHARDWARE = 1,
    BPMEMORY = 2
};

//structs
struct BREAKPOINT
{
    uint addr;
    bool enabled;
    bool singleshoot;
    bool active;
    short oldbytes;
    BP_TYPE type;
    DWORD titantype;
    char name[MAX_BREAKPOINT_SIZE];
    char mod[MAX_MODULE_SIZE];
};

//typedefs
typedef bool (*BPENUMCALLBACK)(const BREAKPOINT* bp);
typedef std::pair<BP_TYPE, uint> BreakpointKey;
typedef std::map<BreakpointKey, BREAKPOINT> BreakpointsInfo;

//functions
int bpgetlist(std::vector<BREAKPOINT>* list);
bool bpnew(uint addr, bool enabled, bool singleshoot, short oldbytes, BP_TYPE type, DWORD titantype, const char* name);
bool bpget(uint addr, BP_TYPE type, const char* name, BREAKPOINT* bp);
bool bpdel(uint addr, BP_TYPE type);
bool bpenable(uint addr, BP_TYPE type, bool enable);
bool bpsetname(uint addr, BP_TYPE type, const char* name);
bool bpsettitantype(uint addr, BP_TYPE type, int titantype);
bool bpenumall(BPENUMCALLBACK cbEnum);
bool bpenumall(BPENUMCALLBACK cbEnum, const char* module);
int bpgetcount(BP_TYPE type, bool enabledonly = false);
void bptobridge(const BREAKPOINT* bp, BRIDGEBP* bridge);
void bpcachesave(JSON root);
void bpcacheload(JSON root);
void bpclear();

#endif // _BREAKPOINT_H

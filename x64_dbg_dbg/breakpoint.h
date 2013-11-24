#ifndef _BREAKPOINT_H
#define _BREAKPOINT_H

#include "_global.h"
#include "TitanEngine\TitanEngine.h"

//enums
enum BP_TYPE
{
    BPNORMAL,
    BPHARDWARE,
    BPMEMORY
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
    char mod[32];
};

//typedefs
typedef bool (*BPENUMCALLBACK)(const BREAKPOINT* bp);

//functions
int bpgetlist(BREAKPOINT** list);
bool bpnew(uint addr, bool enabled, bool singleshoot, short oldbytes, BP_TYPE type, DWORD titantype, const char* name);
bool bpget(uint addr, BP_TYPE type, const char* name, BREAKPOINT* bp);
bool bpdel(uint addr, BP_TYPE type);
bool bpenable(uint addr, BP_TYPE type, bool enable);
bool bpsetname(uint addr, BP_TYPE type, const char* name);
bool bpenumall(BPENUMCALLBACK cbEnum);
bool bpenumall(BPENUMCALLBACK cbEnum, const char* module);
int bpgetcount(BP_TYPE type);
void bpfixmemory(uint addr, unsigned char* dest, uint size);
void bptobridge(const BREAKPOINT* bp, BRIDGEBP* bridge);

#endif // _BREAKPOINT_H

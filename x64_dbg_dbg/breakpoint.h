#ifndef _BREAKPOINT_H
#define _BREAKPOINT_H

#include "_global.h"
#include "TitanEngine\TitanEngine.h"

#define MAX_BREAKPOINT_NAME 256

//enums
enum BP_TYPE
{
    BPNORMAL,
    BPHARDWARE,
    BPMEMORY
};

//structs
#pragma pack(1)
struct BREAKPOINT
{
    uint addr;
    bool enabled;
    bool singleshoot;
    short oldbytes;
    BP_TYPE type;
    int titantype;
    char name[MAX_BREAKPOINT_NAME];
    char mod[32];
};
#pragma pack()

//typedefs
typedef bool (*BPENUMCALLBACK)(const BREAKPOINT* bp);

//functions
int bpgetlist(BREAKPOINT** list);
bool bpnew(uint addr, bool enabled, bool singleshoot, short oldbytes, BP_TYPE type, int titantype, const char* name);
bool bpget(uint addr, BP_TYPE type, const char* name, BREAKPOINT* bp);
bool bpdel(uint addr, BP_TYPE type);
bool bpenable(uint addr, BP_TYPE type, bool enable);
bool bpsetname(uint addr, BP_TYPE type, const char* name);
bool bpenumall(BPENUMCALLBACK cbEnum);
bool bpenumall(BPENUMCALLBACK cbEnum, const char* module);
int bpgetcount(BP_TYPE type);

#endif // _BREAKPOINT_H

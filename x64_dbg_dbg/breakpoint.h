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
};
#pragma pack()

//typedefs
typedef void (*BPENUMCALLBACK)(const BREAKPOINT* bp);

//functions
int bpgetlist(uint** list, int** type);
bool bpnew(uint addr, bool enabled, bool singleshoot, short oldbytes, BP_TYPE type, int titantype);
bool bpget(uint addr, BP_TYPE type, BREAKPOINT* bp);
bool bpdel(uint addr, BP_TYPE type);
bool bpenable(uint addr, BP_TYPE type, bool enable);
bool bpsetname(uint addr, BP_TYPE type, const char* name);
bool bpenumall(BPENUMCALLBACK cbEnum);
bool bpenumall(BPENUMCALLBACK cbEnum, const char* module);

#endif // _BREAKPOINT_H

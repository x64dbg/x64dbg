#ifndef _REFERENCE_H
#define _REFERENCE_H

#include "_global.h"
#include "disasm_fast.h"

//structs
struct REFINFO
{
    int refcount;
    void* userinfo;
    const char* name;
};

//typedefs
typedef bool (*CBREF)(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo);

//functions
int reffind(uint page, uint size, CBREF cbRef, void* userinfo, bool silent, const char* name);

#endif //_REFERENCE_H

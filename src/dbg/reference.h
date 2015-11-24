#pragma once

#include "_global.h"
#include "disasm_fast.h"

struct REFINFO
{
    int refcount;
    void* userinfo;
    const char* name;
};

// Reference callback typedef
typedef bool (*CBREF)(Capstone* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo);

int RefFind(duint Address, duint Size, CBREF Callback, void* UserData, bool Silent, const char* Name);
#ifndef _REFERENCE_H
#define _REFERENCE_H

#include "_global.h"
#include "disasm_fast.h"
#include <functional>

struct REFINFO
{
    int refcount;
    void* userinfo;
    const char* name;
};

typedef enum
{
    CURRENT_REGION,
    CURRENT_MODULE,
    ALL_MODULES
} REFFINDTYPE;

// Reference callback typedef
typedef bool (*CBREF)(Zydis* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo);
typedef std::function<void(int)> CBPROGRESS;

int RefFind(duint Address, duint Size, CBREF Callback, void* UserData, bool Silent, const char* Name, REFFINDTYPE type, bool disasmText);
int RefFindInRange(duint scanStart, duint scanSize, CBREF Callback, void* UserData, bool Silent, REFINFO & refInfo, Zydis & cp, bool initCallBack, const CBPROGRESS & cbUpdateProgress, bool disasmText);

#endif // _REFERENCE_H
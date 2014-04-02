#ifndef _DISASM_FAST_H

#include "_global.h"
#include "BeaEngine\BeaEngine.h"

#define TYPE_VALUE 1
#define TYPE_MEMORY 2
#define TYPE_ADDR 4

#define MAX_MNEMONIC_SIZE 64

enum MEMORY_SIZE
{
    size_byte,
    size_word,
    size_dword,
    size_qword
};

typedef MEMORY_SIZE VALUE_SIZE;

struct MEMORY_INFO
{
    ULONG_PTR value; //displacement / addrvalue (rip-relative)
    MEMORY_SIZE size; //byte/word/dword/qword
    char mnemonic[MAX_MNEMONIC_SIZE];
};

struct VALUE_INFO
{
    ULONG_PTR value;
    VALUE_SIZE size;
};

struct BASIC_INSTRUCTION_INFO
{
    DWORD type; //value|memory|addr
    VALUE_INFO value; //immediat
    MEMORY_INFO memory;
    ULONG_PTR addr; //addrvalue (jumps + calls)
    bool branch; //jumps/calls
};

void fillbasicinfo(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo);

#endif //_DISASM_FAST_H
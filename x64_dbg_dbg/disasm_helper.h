#ifndef _DISASM_HELPER_H
#define _DISASM_HELPER_H

#include "_global.h"

//enums
enum DISASM_INSTRTYPE
{
    instr_normal,
    instr_branch,
    instr_stack
};

enum DISASM_ARGTYPE
{
    arg_normal,
    arg_memory
};

enum STRING_TYPE
{
    str_none,
    str_ascii,
    str_unicode
};

//structures
struct DISASM_ARG
{
    DISASM_ARGTYPE type;
    SEGMENTREG segment;
    char mnemonic[64];
    uint constant;
    uint value;
    uint memvalue;
};


struct DISASM_INSTR
{
    char instruction[64];
    DISASM_INSTRTYPE type;
    int argcount;
	int instr_size;
    DISASM_ARG arg[3];
};

//functions
const char* disasmtext(uint addr);
void disasmprint(uint addr);
void disasmget(unsigned char* buffer, uint addr, DISASM_INSTR* instr);
void disasmget(uint addr, DISASM_INSTR* instr);
bool disasmgetstringat(uint addr, STRING_TYPE* type, char* ascii, char* unicode, int maxlen);

#endif // _DISASM_HELPER_H

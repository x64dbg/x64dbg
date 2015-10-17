#ifndef _DISASM_FAST_H
#define _DISASM_FAST_H

#include "_global.h"
#include "capstone_wrapper.h"

void fillbasicinfo(Capstone* disasm, BASIC_INSTRUCTION_INFO* basicinfo);
bool disasmfast(uint addr, BASIC_INSTRUCTION_INFO* basicinfo);
bool disasmfast(unsigned char* data, uint addr, BASIC_INSTRUCTION_INFO* basicinfo);

#endif //_DISASM_FAST_H

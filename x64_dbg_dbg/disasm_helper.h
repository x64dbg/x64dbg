#ifndef _DISASM_HELPER_H
#define _DISASM_HELPER_H

#include "_global.h"

//functions
unsigned int disasmback(unsigned char* data, uint base, uint size, uint ip, int n);
unsigned int disasmnext(unsigned char* data, uint base, uint size, uint ip, int n);
const char* disasmtext(uint addr);
void disasmprint(uint addr);
void disasmget(unsigned char* buffer, uint addr, DISASM_INSTR* instr);
void disasmget(uint addr, DISASM_INSTR* instr);
bool disasmgetstringat(uint addr, STRING_TYPE* type, char* ascii, char* unicode, int maxlen);

#endif // _DISASM_HELPER_H

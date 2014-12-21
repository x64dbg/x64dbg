#ifndef _DISASM_HELPER_H
#define _DISASM_HELPER_H

#include "_global.h"

//functions
uint disasmback(unsigned char* data, uint base, uint size, uint ip, int n);
uint disasmnext(unsigned char* data, uint base, uint size, uint ip, int n);
const char* disasmtext(uint addr);
void disasmprint(uint addr);
void disasmget(unsigned char* buffer, uint addr, DISASM_INSTR* instr);
void disasmget(uint addr, DISASM_INSTR* instr);
bool disasmispossiblestring(uint addr);
bool disasmgetstringat(uint addr, STRING_TYPE* type, char* ascii, char* unicode, int maxlen);
int disasmgetsize(uint addr, unsigned char* data);
int disasmgetsize(uint addr);

#endif // _DISASM_HELPER_H

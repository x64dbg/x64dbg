#ifndef _DISASM_HELPER_H
#define _DISASM_HELPER_H

#include "_global.h"

//functions
duint disasmback(unsigned char* data, duint base, duint size, duint ip, int n);
duint disasmnext(unsigned char* data, duint base, duint size, duint ip, int n);
const char* disasmtext(duint addr);
void disasmprint(duint addr);
void disasmget(unsigned char* buffer, duint addr, DISASM_INSTR* instr);
void disasmget(duint addr, DISASM_INSTR* instr);
bool disasmispossiblestring(duint addr);
bool disasmgetstringat(duint addr, STRING_TYPE* type, char* ascii, char* unicode, int maxlen);
int disasmgetsize(duint addr, unsigned char* data);
int disasmgetsize(duint addr);

#endif // _DISASM_HELPER_H

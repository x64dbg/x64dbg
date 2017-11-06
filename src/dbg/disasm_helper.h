#ifndef _DISASM_HELPER_H
#define _DISASM_HELPER_H

#include "_global.h"
#include "zydis_wrapper.h"

//functions
duint disasmback(unsigned char* data, duint base, duint size, duint ip, int n);
duint disasmnext(unsigned char* data, duint base, duint size, duint ip, int n);
void disasmget(Zydis & cp, unsigned char* buffer, duint addr, DISASM_INSTR* instr, bool getregs = true);
void disasmget(Zydis & cp, duint addr, DISASM_INSTR* instr, bool getregs = true);
void disasmget(unsigned char* buffer, duint addr, DISASM_INSTR* instr, bool getregs = true);
void disasmget(duint addr, DISASM_INSTR* instr, bool getregs = true);
bool disasmispossiblestring(duint addr, STRING_TYPE* type = nullptr);
bool disasmgetstringat(duint addr, STRING_TYPE* type, char* ascii, char* unicode, int maxlen);
bool disasmgetstringatwrapper(duint addr, char* text, bool cache = true);
int disasmgetsize(duint addr, unsigned char* data);
int disasmgetsize(duint addr);

#endif // _DISASM_HELPER_H

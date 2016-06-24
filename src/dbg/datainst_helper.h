#pragma once
#include "_global.h"

void initDataInstMap();
bool isdatainstruction(const char* instruction);
bool tryassembledata(duint addr, unsigned char* dest, int destsize, int* size, const char* instruction, char* error);
bool trydisasm(const unsigned char* buffer, duint addr, DISASM_INSTR* instr, duint codesize);
bool trydisasmfast(const unsigned char* buffer, duint addr, BASIC_INSTRUCTION_INFO* basicinfo, duint codesize);
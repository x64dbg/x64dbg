#ifndef _ASSEMBLE_H
#define _ASSEMBLE_H

#include "_global.h"

enum class AssemblerEngine
{
    XEDParse = 0,
    Keystone = 1,
    asmjit = 2
};

extern AssemblerEngine assemblerEngine;

bool assemble(duint addr, unsigned char* dest, int* size, const char* instruction, char* error);
bool assembleat(duint addr, const char* instruction, int* size, char* error, bool fillnop);

#endif // _ASSEMBLE_H

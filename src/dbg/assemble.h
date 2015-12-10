#ifndef _ASSEMBLE_H
#define _ASSEMBLE_H

#include "_global.h"

enum INSTR_POINTING_TO
{
    EX_MEMORY,
    NX_MEMORY,
    NO_POINTER
};

bool assemble(duint addr, unsigned char* dest, int* size, const char* instruction, char* error);
bool assembleat(duint addr, const char* instruction, int* size, char* error, bool fillnop);
INSTR_POINTING_TO isInstructionPointingToExMemory(duint addr, const unsigned char* dest);

#endif // _ASSEMBLE_H

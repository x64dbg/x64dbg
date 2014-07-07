#ifndef _ASSEMBLE_H
#define _ASSEMBLE_H

#include "_global.h"

bool assemble(uint addr, unsigned char* dest, int* size, const char* instruction, char* error);
bool assembleat(uint addr, const char* instruction, int* size, char* error, bool fillnop);

#endif // _ASSEMBLE_H

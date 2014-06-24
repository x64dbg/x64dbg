#ifndef _ASSEMBLE_H
#define _ASSEMBLE_H

#include "_global.h"

bool assembleat(uint addr, const char* instruction, char* error, bool fillnop);

#endif // _ASSEMBLE_H

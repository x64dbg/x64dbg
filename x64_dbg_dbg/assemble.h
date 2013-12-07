#ifndef _ASSEMBLE_H
#define _ASSEMBLE_H

#include "_global.h"

//superglobal variables
extern char nasmpath[deflen];

bool assemble(const char* instruction, unsigned char** outdata, int* outsize, char* error, bool x64);
void intel2nasm(const char* intel, char* nasm);

#endif // _ASSEMBLE_H

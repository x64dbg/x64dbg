#ifndef _MATH_H
#define _MATH_H

#include "_global.h"

int mathisoperator(char ch);
void mathformat(char* text);
bool mathcontains(const char* text);
bool mathhandlebrackets(char* expression);
bool mathfromstring(const char* string, uint* value, int* value_size, bool* isvar);
bool mathdounsignedoperation(char op, uint left, uint right, uint* result);
bool mathdosignedoperation(char op, sint left, sint right, sint* result);

#endif // _MATH_H

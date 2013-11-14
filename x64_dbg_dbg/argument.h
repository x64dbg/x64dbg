#ifndef _ARGUMENT_H
#define _ARGUMENT_H

#include "_global.h"

//functions
bool argget(const char* cmd, char* arg, int arg_num, bool optional);
int arggetcount(const char* cmd);
void argformat(char* cmd);

#endif

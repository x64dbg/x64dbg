#ifndef _STACKINFO_H
#define _STACKINFO_H

#include "_global.h"

struct CALLSTACKENTRY
{
    uint addr;
    uint from;
    uint to;
    char comment[MAX_COMMENT_SIZE];
};

struct CALLSTACK
{
    int total;
    CALLSTACKENTRY* entries;
};

bool stackcommentget(uint addr, STACK_COMMENT* comment);
void stackgetcallstack(uint csp, CALLSTACK* callstack);

#endif //_STACKINFO_H
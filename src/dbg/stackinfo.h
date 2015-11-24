#ifndef _STACKINFO_H
#define _STACKINFO_H

#include "_global.h"

struct CALLSTACKENTRY
{
    duint addr;
    duint from;
    duint to;
    char comment[MAX_COMMENT_SIZE];
};

struct CALLSTACK
{
    int total;
    CALLSTACKENTRY* entries;
};

bool stackcommentget(duint addr, STACK_COMMENT* comment);
void stackgetcallstack(duint csp, CALLSTACK* callstack);

#endif //_STACKINFO_H
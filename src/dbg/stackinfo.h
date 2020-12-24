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

void stackupdateseh();
bool stackcommentget(duint addr, STACK_COMMENT* comment);
void stackupdatecallstack(duint csp);
void stackgetcallstack(duint csp, CALLSTACK* callstack);
void stackgetcallstack(duint csp, std::vector<CALLSTACKENTRY> & callstack, bool cache);
void stackgetcallstackbythread(HANDLE thread, CALLSTACK* callstack);
void stackupdatesettings();

#endif //_STACKINFO_H
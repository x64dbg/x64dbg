#ifndef _THREADING_H
#define _THREADING_H

#include "_global.h"

//enums
enum WAIT_ID
{
    WAITID_SYSBREAK=0,
    WAITID_RUN=1
};

//functions
void waitclear();
void wait(WAIT_ID id);
void lock(WAIT_ID id);
void unlock(WAIT_ID id);
bool waitislocked(WAIT_ID id);

#endif // _THREADING_H

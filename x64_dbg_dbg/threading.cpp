#include "threading.h"

static bool waitarray[16];

void waitclear()
{
    memset(waitarray, 0, sizeof(waitarray));
}

void wait(WAIT_ID id)
{
    while(waitarray[id]) //1=locked, 0=unlocked
        Sleep(1);
}

void lock(WAIT_ID id)
{
    waitarray[id]=true;
}

void unlock(WAIT_ID id)
{
    waitarray[id]=false;
}

bool waitislocked(WAIT_ID id)
{
    return waitarray[id];
}

#include "threading.h"

static volatile bool waitarray[WAITID_LAST];

void waitclear()
{
    memset((void*)waitarray, 0, sizeof(waitarray));
}

void wait(WAIT_ID id)
{
    while(waitarray[id]) //1=locked, 0=unlocked
        Sleep(1);
}

void lock(WAIT_ID id)
{
    waitarray[id] = true;
}

void unlock(WAIT_ID id)
{
    waitarray[id] = false;
}

bool waitislocked(WAIT_ID id)
{
    return waitarray[id];
}

static CRITICAL_SECTION locks[LockLast] = {};
static bool bInitDone = false;

static void CriticalSectionInitializeLocks()
{
    if(bInitDone)
        return;
    for(int i = 0; i < LockLast; i++)
        InitializeCriticalSection(&locks[i]);
    bInitDone = true;
}

void CriticalSectionDeleteLocks()
{
    if(!bInitDone)
        return;
    for(int i = 0; i < LockLast; i++)
        DeleteCriticalSection(&locks[i]);
    bInitDone = false;
}

CriticalSectionLocker::CriticalSectionLocker(CriticalSectionLock lock)
{
    CriticalSectionInitializeLocks(); //initialize critical sections
    gLock = lock;

    EnterCriticalSection(&locks[gLock]);
    Locked = true;
}

CriticalSectionLocker::~CriticalSectionLocker()
{
    if(Locked)
        LeaveCriticalSection(&locks[gLock]);
}

void CriticalSectionLocker::unlock()
{
    Locked = false;
    LeaveCriticalSection(&locks[gLock]);
}

void CriticalSectionLocker::relock()
{
    EnterCriticalSection(&locks[gLock]);
    Locked = true;
}
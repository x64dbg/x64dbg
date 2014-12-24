/**
 @file threading.cpp

 @brief Implements various functions for syncing threads.
 */

#include "threading.h"

/**
\brief The lock values.
*/
static volatile bool waitarray[WAITID_LAST];

/**
\brief Sets all lock values to 0.
*/
void waitclear()
{
    memset((void*)waitarray, 0, sizeof(waitarray));
}

/**
\brief Waits while a lock is 1.
\param id The lock to wait for.
*/
void wait(WAIT_ID id)
{
    while(waitarray[id]) //1=locked, 0=unlocked
        Sleep(1);
}

/**
\brief Sets a lock to 1.
\param id The lock to set.
*/
void lock(WAIT_ID id)
{
    waitarray[id] = true;
}

/**
\brief Sets a lock to 0.
\param id The lock to set.
*/
void unlock(WAIT_ID id)
{
    waitarray[id] = false;
}

/**
\brief Returns the lock value.
\param id The lock to check.
\return true if locked, false otherwise.
*/
bool waitislocked(WAIT_ID id)
{
    return waitarray[id];
}

CRITICAL_SECTION CriticalSectionLocker::locks[LockLast] = {};
bool CriticalSectionLocker::bInitDone = false;

void CriticalSectionLocker::Initialize()
{
    if(bInitDone)
        return;
    for(int i = 0; i < LockLast; i++)
        InitializeCriticalSection(&locks[i]);
    bInitDone = true;
}

void CriticalSectionLocker::Deinitialize()
{
    if(!bInitDone)
        return;
    for(int i = 0; i < LockLast; i++)
    {
        EnterCriticalSection(&locks[i]); //obtain ownership
        DeleteCriticalSection(&locks[i]);
    }
    bInitDone = false;
}

CriticalSectionLocker::CriticalSectionLocker(CriticalSectionLock lock)
{
    Initialize(); //initialize critical sections
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
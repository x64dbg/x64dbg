/**
 @file threading.cpp

 @brief Implements the threading class.
 */

#include "threading.h"

/**
 @property static volatile bool waitarray[16]

 @brief Gets a value indicating whether the waitarray[ 16].

 @return true if waitarray[ 16], false if not.
 */

static volatile bool waitarray[16];

/**
 @fn void waitclear()

 @brief Waitclears this object.
 */

void waitclear()
{
    memset((void*)waitarray, 0, sizeof(waitarray));
}

/**
 @fn void wait(WAIT_ID id)

 @brief Waits the given identifier.

 @param id The identifier.
 */

void wait(WAIT_ID id)
{
    while(waitarray[id]) //1=locked, 0=unlocked
        Sleep(1);
}

/**
 @fn void lock(WAIT_ID id)

 @brief Locks the given identifier.

 @param id The identifier.
 */

void lock(WAIT_ID id)
{
    waitarray[id] = true;
}

/**
 @fn void unlock(WAIT_ID id)

 @brief Unlocks the given identifier.

 @param id The identifier.
 */

void unlock(WAIT_ID id)
{
    waitarray[id] = false;
}

/**
 @fn bool waitislocked(WAIT_ID id)

 @brief Waitislocked the given identifier.

 @param id The identifier.

 @return true if it succeeds, false if it fails.
 */

bool waitislocked(WAIT_ID id)
{
    return waitarray[id];
}

/**
 @brief The locks[ lock last].
 */

static CRITICAL_SECTION locks[LockLast] = {};

/**
 @brief The initialise done.
 */

static bool bInitDone = false;

/**
 @fn static void CriticalSectionInitializeLocks()

 @brief Critical section initialize locks.
 */

static void CriticalSectionInitializeLocks()
{
    if(bInitDone)
        return;
    for(int i = 0; i < LockLast; i++)
        InitializeCriticalSection(&locks[i]);
    bInitDone = true;
}

/**
 @fn void CriticalSectionDeleteLocks()

 @brief Critical section delete locks.
 */

void CriticalSectionDeleteLocks()
{
    if(!bInitDone)
        return;
    for(int i = 0; i < LockLast; i++)
        DeleteCriticalSection(&locks[i]);
    bInitDone = false;
}

/**
 @fn CriticalSectionLocker::CriticalSectionLocker(CriticalSectionLock lock)

 @brief Constructor.

 @param lock The lock.
 */

CriticalSectionLocker::CriticalSectionLocker(CriticalSectionLock lock)
{
    CriticalSectionInitializeLocks(); //initialize critical sections
    gLock = lock;
    EnterCriticalSection(&locks[gLock]);
}

/**
 @fn CriticalSectionLocker::~CriticalSectionLocker()

 @brief Destructor.
 */

CriticalSectionLocker::~CriticalSectionLocker()
{
    LeaveCriticalSection(&locks[gLock]);
}

/**
 @fn void CriticalSectionLocker::unlock()

 @brief Unlocks this object.
 */

void CriticalSectionLocker::unlock()
{
    LeaveCriticalSection(&locks[gLock]);
}

/**
 @fn void CriticalSectionLocker::relock()

 @brief Relocks this object.
 */

void CriticalSectionLocker::relock()
{
    EnterCriticalSection(&locks[gLock]);
}
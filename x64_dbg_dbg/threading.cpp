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

bool SectionLockerGlobal::m_Initialized = false;
SRWLOCK SectionLockerGlobal::m_Locks[SectionLock::LockLast];

void SectionLockerGlobal::Initialize()
{
    if(m_Initialized)
        return;

    // Destroy previous data if any existed
    memset(m_Locks, 0, sizeof(m_Locks));

    for(int i = 0; i < ARRAYSIZE(m_Locks); i++)
        InitializeSRWLock(&m_Locks[i]);

    m_Initialized = true;
}

void SectionLockerGlobal::Deinitialize()
{
    if(!m_Initialized)
        return;

    for(int i = 0; i < ARRAYSIZE(m_Locks); i++)
    {
        // Wait for the lock's ownership to be released
        AcquireSRWLockExclusive(&m_Locks[i]);
        ReleaseSRWLockExclusive(&m_Locks[i]);

        // Invalidate data
        memset(&m_Locks[i], 0, sizeof(SRWLOCK));
    }

    m_Initialized = false;
}
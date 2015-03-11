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

bool CriticalSectionLocker::m_Initialized = false;
SRWLOCK CriticalSectionLocker::m_Locks[LockLast];

void CriticalSectionLocker::Initialize()
{
    if(m_Initialized)
        return;

    // Destroy previous data if any existed
    memset(m_Locks, 0, sizeof(m_Locks));

    for(int i = 0; i < LockLast; i++)
        InitializeSRWLock(&m_Locks[i]);

    m_Initialized = true;
}

void CriticalSectionLocker::Deinitialize()
{
    if(!m_Initialized)
        return;

    for(int i = 0; i < LockLast; i++)
    {
        // Wait for the lock's ownership to be released
        AcquireSRWLockExclusive(&m_Locks[i]);
        ReleaseSRWLockExclusive(&m_Locks[i]);

        // Invalidate data
        memset(&m_Locks[i], 0, sizeof(SRWLOCK));
    }

    m_Initialized = false;
}

CriticalSectionLocker::CriticalSectionLocker(CriticalSectionLock LockIndex, bool Shared)
{
    m_Lock      = &m_Locks[LockIndex];
    m_LockCount = 0;

    Lock(Shared);
}

CriticalSectionLocker::~CriticalSectionLocker()
{
    if(m_LockCount > 0)
        Unlock();

    // TODO: Assert that the lock count is zero on destructor
}

void CriticalSectionLocker::Unlock()
{
    m_LockCount--;

    if(m_Shared)
        ReleaseSRWLockShared(m_Lock);
    else
        ReleaseSRWLockExclusive(m_Lock);
}

void CriticalSectionLocker::Lock(bool Shared)
{
    if(Shared)
        AcquireSRWLockShared(m_Lock);
    else
        AcquireSRWLockExclusive(m_Lock);

    m_Shared = Shared;
    m_LockCount++;
}
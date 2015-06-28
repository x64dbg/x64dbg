#include "threading.h"

static HANDLE waitArray[WAITID_LAST];

void waitclear()
{
    for(int i = 0; i < WAITID_LAST; i++)
        unlock((WAIT_ID)i);
}

void wait(WAIT_ID id)
{
    WaitForSingleObject(waitArray[id], INFINITE);
}

void lock(WAIT_ID id)
{
    ResetEvent(waitArray[id]);
}

void unlock(WAIT_ID id)
{
    SetEvent(waitArray[id]);
}

bool waitislocked(WAIT_ID id)
{
    return !WaitForSingleObject(waitArray[id], 0) == WAIT_OBJECT_0;
}

void waitinitialize()
{
    for(int i = 0; i < WAITID_LAST; i++)
        waitArray[i] = CreateEventA(NULL, TRUE, TRUE, NULL);
}

void waitdeinitialize()
{
    for(int i = 0; i < WAITID_LAST; i++)
    {
        wait((WAIT_ID)i);
        CloseHandle(waitArray[i]);
    }
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
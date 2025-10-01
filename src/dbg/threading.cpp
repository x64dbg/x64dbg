#include <ntstatus.h>
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

bool waitfor(WAIT_ID id, unsigned int Milliseconds)
{
    return WaitForSingleObject(waitArray[id], Milliseconds) == 0;
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
    return !(WaitForSingleObject(waitArray[id], 0) == WAIT_OBJECT_0);
}

void waitinitialize()
{
    for(int i = 0; i < WAITID_LAST; i++)
        waitArray[i] = CreateEventW(NULL, TRUE, TRUE, NULL);
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

CacheAligned<CRITICAL_SECTION> SectionLockerGlobal::m_crLocks[SectionLock::LockLast];
CacheAligned<SRWLOCK> SectionLockerGlobal::m_srwLocks[SectionLock::LockLast];
SectionLockerGlobal::owner_info SectionLockerGlobal::m_exclusiveOwner[SectionLock::LockLast];
DWORD SectionLockerGlobal::m_guiMainThreadId;

void SectionLockerGlobal::Initialize()
{
    // This is supposed to only be called once, but
    // create a flag anyway
    if(m_Initialized)
        return;

    // This gets called on the same thread as the GUI
    m_guiMainThreadId = GetCurrentThreadId();

    // Destroy previous data if any existed
    memset(m_srwLocks, 0, sizeof(m_srwLocks));

    for(int i = 0; i < ARRAYSIZE(m_srwLocks); i++)
        InitializeSRWLock(&m_srwLocks[i]);

    m_Initialized = true;
}

void SectionLockerGlobal::Deinitialize()
{
    if(!m_Initialized)
        return;

    for(int i = 0; i < ARRAYSIZE(m_srwLocks); i++)
    {
        // Wait for the lock's ownership to be released
        AcquireSRWLockExclusive(&m_srwLocks[i]);
        ReleaseSRWLockExclusive(&m_srwLocks[i]);

        // Invalidate data
        memset(&m_srwLocks[i], 0, sizeof(SRWLOCK));
    }

    m_Initialized = false;
}

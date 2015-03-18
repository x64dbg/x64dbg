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

bool ExclusiveSectionLocker::m_Initialized = false;
SRWLOCK ExclusiveSectionLocker::m_Locks[SectionLock::LockLast];

void ExclusiveSectionLocker::Initialize()
{
    if(m_Initialized)
        return;

    // Destroy previous data if any existed
    memset(m_Locks, 0, sizeof(m_Locks));

    for(int i = 0; i < ARRAYSIZE(m_Locks); i++)
        InitializeSRWLock(&m_Locks[i]);

    m_Initialized = true;
}

void ExclusiveSectionLocker::Deinitialize()
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

ExclusiveSectionLocker::ExclusiveSectionLocker(SectionLock LockIndex)
{
    m_Lock      = &m_Locks[LockIndex];
    m_LockCount = 0;

    Lock();
}

ExclusiveSectionLocker::~ExclusiveSectionLocker()
{
    if(m_LockCount > 0)
        Unlock();

    // TODO: Assert that the lock count is zero on destructor
#ifdef _DEBUG
    if(m_LockCount > 0)
        __debugbreak();
#endif
}

void ExclusiveSectionLocker::Lock()
{
    AcquireSRWLockExclusive(m_Lock);

    m_LockCount++;
}

void ExclusiveSectionLocker::Unlock()
{
    m_LockCount--;

    ReleaseSRWLockExclusive(m_Lock);
}

SharedSectionLocker::SharedSectionLocker(SectionLock LockIndex)
    : ExclusiveSectionLocker(LockIndex)
{
    // Nothing to do here; parent class constructor is called
}

void SharedSectionLocker::Lock()
{
    AcquireSRWLockShared(m_Lock);

    m_LockCount++;
}

void SharedSectionLocker::Unlock()
{
    m_LockCount--;

    ReleaseSRWLockShared(m_Lock);
}
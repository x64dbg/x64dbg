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
CRITICAL_SECTION CriticalSectionLocker::m_Locks[LockLast];

void CriticalSectionLocker::Initialize()
{
    if(m_Initialized)
        return;

    // Destroy previous data if any existed
    memset(m_Locks, 0, sizeof(m_Locks));

    for(int i = 0; i < LockLast; i++)
        InitializeCriticalSection(&m_Locks[i]);

    m_Initialized = true;
}

void CriticalSectionLocker::Deinitialize()
{
    if(!m_Initialized)
        return;

    for(int i = 0; i < LockLast; i++)
    {
        // Wait for the lock's ownership to be released
        EnterCriticalSection(&m_Locks[i]);
        LeaveCriticalSection(&m_Locks[i]);

        // Render the lock data invalid
        DeleteCriticalSection(&m_Locks[i]);
    }

    m_Initialized = false;
}

CriticalSectionLocker::CriticalSectionLocker(CriticalSectionLock LockIndex)
{
    m_Section   = &m_Locks[LockIndex];
    m_LockCount = 0;

    Lock();
}

CriticalSectionLocker::~CriticalSectionLocker()
{
    if(m_LockCount > 0)
        LeaveCriticalSection(m_Section);

    // TODO: Assert that the lock count is zero on destructor
}

void CriticalSectionLocker::Unlock()
{
    m_LockCount--;
    LeaveCriticalSection(m_Section);
}

void CriticalSectionLocker::Lock()
{
    EnterCriticalSection(m_Section);
    m_LockCount++;
}

bool CriticalSectionLocker::TryLock()
{
    // Only enter the critical section if it's currently owned by the
    // thread, or if it is not being used at all
    if(TryEnterCriticalSection(m_Section))
    {
        Lock();
        return true;
    }

    return false;
}
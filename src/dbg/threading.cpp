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
bool SectionLockerGlobal::m_SRWLocks = false;
CacheAligned<SRWLOCK> SectionLockerGlobal::m_srwLocks[SectionLock::LockLast];
SectionLockerGlobal::owner_info SectionLockerGlobal::m_exclusiveOwner[SectionLock::LockLast];

CacheAligned<CRITICAL_SECTION> SectionLockerGlobal::m_crLocks[SectionLock::LockLast];
SectionLockerGlobal::SRWLOCKFUNCTION SectionLockerGlobal::m_InitializeSRWLock;
SectionLockerGlobal::SRWLOCKFUNCTION SectionLockerGlobal::m_AcquireSRWLockShared;
SectionLockerGlobal::TRYSRWLOCKFUNCTION SectionLockerGlobal::m_TryAcquireSRWLockShared;
SectionLockerGlobal::SRWLOCKFUNCTION SectionLockerGlobal::m_AcquireSRWLockExclusive;
SectionLockerGlobal::TRYSRWLOCKFUNCTION SectionLockerGlobal::m_TryAcquireSRWLockExclusive;
SectionLockerGlobal::SRWLOCKFUNCTION SectionLockerGlobal::m_ReleaseSRWLockShared;
SectionLockerGlobal::SRWLOCKFUNCTION SectionLockerGlobal::m_ReleaseSRWLockExclusive;
DWORD SectionLockerGlobal::m_guiMainThreadId;

void SectionLockerGlobal::Initialize()
{
    // This is supposed to only be called once, but
    // create a flag anyway
    if(m_Initialized)
        return;

    // This gets called on the same thread as the GUI
    m_guiMainThreadId = GetCurrentThreadId();

    // Attempt to read the SRWLock API
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    m_InitializeSRWLock = (SRWLOCKFUNCTION)GetProcAddress(hKernel32, "InitializeSRWLock");
    m_AcquireSRWLockShared = (SRWLOCKFUNCTION)GetProcAddress(hKernel32, "AcquireSRWLockShared");
    m_TryAcquireSRWLockShared = (TRYSRWLOCKFUNCTION)GetProcAddress(hKernel32, "TryAcquireSRWLockShared");
    m_AcquireSRWLockExclusive = (SRWLOCKFUNCTION)GetProcAddress(hKernel32, "AcquireSRWLockExclusive");
    m_TryAcquireSRWLockExclusive = (TRYSRWLOCKFUNCTION)GetProcAddress(hKernel32, "TryAcquireSRWLockExclusive");
    m_ReleaseSRWLockShared = (SRWLOCKFUNCTION)GetProcAddress(hKernel32, "ReleaseSRWLockShared");
    m_ReleaseSRWLockExclusive = (SRWLOCKFUNCTION)GetProcAddress(hKernel32, "ReleaseSRWLockExclusive");

    m_SRWLocks = m_InitializeSRWLock &&
                 m_AcquireSRWLockShared &&
                 m_TryAcquireSRWLockShared &&
                 m_AcquireSRWLockExclusive &&
                 m_TryAcquireSRWLockExclusive &&
                 m_ReleaseSRWLockShared &&
                 m_ReleaseSRWLockExclusive;

    if(m_SRWLocks) // Prefer SRWLocks
    {
        // Destroy previous data if any existed
        memset(m_srwLocks, 0, sizeof(m_srwLocks));

        for(int i = 0; i < ARRAYSIZE(m_srwLocks); i++)
            m_InitializeSRWLock(&m_srwLocks[i]);
    }
    else // Fall back to critical sections otherwise
    {
        // Destroy previous data if any existed
        memset(m_crLocks, 0, sizeof(m_crLocks));

        for(int i = 0; i < ARRAYSIZE(m_crLocks); i++)
            InitializeCriticalSection(&m_crLocks[i]);
    }

    m_Initialized = true;
}

void SectionLockerGlobal::Deinitialize()
{
    if(!m_Initialized)
        return;

    if(m_SRWLocks)
    {
        for(int i = 0; i < ARRAYSIZE(m_srwLocks); i++)
        {
            // Wait for the lock's ownership to be released
            m_AcquireSRWLockExclusive(&m_srwLocks[i]);
            m_ReleaseSRWLockExclusive(&m_srwLocks[i]);

            // Invalidate data
            memset(&m_srwLocks[i], 0, sizeof(SRWLOCK));
        }
    }
    else
    {
        for(int i = 0; i < ARRAYSIZE(m_crLocks); i++)
        {
            // Wait for the lock's ownership to be released
            EnterCriticalSection(&m_crLocks[i]);
            LeaveCriticalSection(&m_crLocks[i]);

            // Delete critical section
            DeleteCriticalSection(&m_crLocks[i]);
            memset(&m_crLocks[i], 0, sizeof(CRITICAL_SECTION));
        }
    }

    m_Initialized = false;
}

static DWORD gTlsIndex = TLS_OUT_OF_INDEXES;

TLSData::TLSData()
{
    moduleHashLower.reserve(MAX_MODULE_SIZE);
}

bool TLSData::notify(DWORD fdwReason)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        gTlsIndex = TlsAlloc();
        return gTlsIndex != TLS_OUT_OF_INDEXES;

    case DLL_THREAD_DETACH:
    {
        auto data = (TLSData*)TlsGetValue(gTlsIndex);
        delete data;
    }
    return true;

    case DLL_PROCESS_DETACH:
    {
        auto data = (TLSData*)TlsGetValue(gTlsIndex);
        delete data;
        TlsFree(gTlsIndex);
    }
    return true;
    }

    return false;
}

TLSData* TLSData::get()
{
    auto data = (TLSData*)TlsGetValue(gTlsIndex);
    if(data == nullptr)
    {
        data = new TLSData();
        TlsSetValue(gTlsIndex, data);
    }
    return data;
}

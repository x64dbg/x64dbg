#pragma once

#include "_global.h"

enum WAIT_ID
{
    WAITID_RUN,
    WAITID_STOP,
    WAITID_LAST
};

//functions
void waitclear();
void wait(WAIT_ID id);
void lock(WAIT_ID id);
void unlock(WAIT_ID id);
bool waitislocked(WAIT_ID id);

//
// THREAD SYNCHRONIZATION
//
// Better, but requires VISTA+
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa904937%28v=vs.85%29.aspx
//
#define CriticalSectionLocker
#define locker(x) EXCLUSIVE_ACQUIRE(x)

#define EXCLUSIVE_ACQUIRE(Index)    SectionLocker<false> __ThreadLock(SectionLock::##Index);
#define EXCLUSIVE_RELEASE()         __ThreadLock.Unlock();

#define SHARED_ACQUIRE(Index)       SectionLocker<true> __SThreadLock(SectionLock::##Index);
#define SHARED_RELEASE()            __SThreadLock.Unlock();

enum SectionLock
{
    LockMemoryPages,
    LockVariables,
    LockModules,
    LockComments,
    LockLabels,
    LockBookmarks,
    LockFunctions,
    LockLoops,
    LockBreakpoints,
    LockPatches,
    LockThreads,
    LockDprintf,

    // This is defined because of a bug in the Windows 8.1 kernel;
    // Calling VirtualQuery/VirtualProtect/ReadProcessMemory can and will cause
    // a deadlock.
    // https://bitbucket.org/mrexodia/x64_dbg/issue/247/x64-dbg-bug-string-references-function
    LockWin8Workaround,

    LockLast
};

class SectionLockerGlobal
{
    template<bool Shared> friend class SectionLocker;

public:
    static void Initialize();
    static void Deinitialize();

protected:
    static bool     m_Initialized;
    static SRWLOCK  m_Locks[SectionLock::LockLast];
};

template<bool Shared>
class SectionLocker
{
public:
    SectionLocker(SectionLock LockIndex)
    {
        m_Lock      = &SectionLockerGlobal::m_Locks[LockIndex];
        m_LockCount = 0;

        Lock();
    }

    ~SectionLocker()
    {
        if(m_LockCount > 0)
            Unlock();

        // TODO: Assert that the lock count is zero on destructor
#ifdef _DEBUG
        if(m_LockCount > 0)
            __debugbreak();
#endif
    }

    inline void Lock()
    {
        if(Shared)
            AcquireSRWLockShared(m_Lock);
        else
            AcquireSRWLockExclusive(m_Lock);

        m_LockCount++;
    }

    inline void Unlock()
    {
        m_LockCount--;

        if(Shared)
            ReleaseSRWLockShared(m_Lock);
        else
            ReleaseSRWLockExclusive(m_Lock);
    }

protected:
    PSRWLOCK    m_Lock;
    BYTE        m_LockCount;
};
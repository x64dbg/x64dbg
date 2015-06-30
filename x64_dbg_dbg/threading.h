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
void waitinitialize();
void waitdeinitialize();

//
// THREAD SYNCHRONIZATION
//
// Better, but requires VISTA+
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa904937%28v=vs.85%29.aspx
//
#define EXCLUSIVE_ACQUIRE(Index)    SectionLocker<SectionLock::##Index, false> __ThreadLock;
#define EXCLUSIVE_RELEASE()         __ThreadLock.Unlock();

#define SHARED_ACQUIRE(Index)       SectionLocker<SectionLock::##Index, true> __SThreadLock;
#define SHARED_REACQUIRE(Index)     __SThreadLock.Lock();
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
    LockSym,

    // This is defined because of a bug in the Windows 8.1 kernel;
    // Calling VirtualQuery/VirtualProtect/ReadProcessMemory can and will cause
    // a deadlock.
    // https://bitbucket.org/mrexodia/x64_dbg/issue/247/x64-dbg-bug-string-references-function
    LockWin8Workaround,

    LockLast,
};

class SectionLockerGlobal
{
    template<SectionLock LockIndex, bool Shared> friend class SectionLocker;

public:
    static void Initialize();
    static void Deinitialize();

private:
    static bool     m_Initialized;
    static SRWLOCK  m_Locks[SectionLock::LockLast];
};

template<SectionLock LockIndex, bool Shared>
class SectionLocker
{
public:
    SectionLocker()
    {
        m_LockCount = 0;
        Lock();
    }

    ~SectionLocker()
    {
        if(m_LockCount > 0)
            Unlock();

#ifdef _DEBUG
        // TODO: Assert that the lock count is zero on destructor
        if(m_LockCount > 0)
            __debugbreak();
#endif
    }

    inline void Lock()
    {
        if(Shared)
            AcquireSRWLockShared(&Internal::m_Locks[LockIndex]);
        else
            AcquireSRWLockExclusive(&Internal::m_Locks[LockIndex]);

        m_LockCount++;
    }

    inline void Unlock()
    {
        m_LockCount--;

        if(Shared)
            ReleaseSRWLockShared(&Internal::m_Locks[LockIndex]);
        else
            ReleaseSRWLockExclusive(&Internal::m_Locks[LockIndex]);
    }

private:
    using Internal = SectionLockerGlobal;

protected:
    BYTE m_LockCount;
};